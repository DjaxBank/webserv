#include <string>
#include <iostream>
#include <requestParser.hpp>
#include <Request.hpp>
#include <array>
#include <string_view>


bool RequestParser::errorOnScheme(const std::string& copy_uri)
{
	size_t pos;

	pos = copy_uri.find(HTTP_CONSTANT::SCHEME_SUFFIX);
	if (pos != std::string::npos)
	{
		m_state = ParserState::ERROR;
		throw HttpParseException(
            ParseError::InvalidUriSyntax, 
            ReplyStatus::BadRequest,
            "Malformed request: scheme detected."    
        );
	}
	return true;
}

bool RequestParser::errorOnAuthority(const std::string& copy_uri)
{
	if (copy_uri.compare(0, 2, "//") == 0)
	{
		m_state = ParserState::ERROR;
		throw HttpParseException(
            ParseError::InvalidUriSyntax, 
            ReplyStatus::BadRequest,
            "Malformed request: Authority detected/incorrect file path."    
        );
	}
	return true;
}

bool RequestParser::errorOnUserInfo(const std::string& copy_uri)
{
	size_t pos;

	pos = copy_uri.find('/', 1);
	if (pos == std::string::npos)
		return true;
	std::string buffer(copy_uri.substr(0, pos));
	pos = buffer.find('@');
	if (pos != std::string::npos)
	{
		m_state = ParserState::ERROR;
		throw HttpParseException(
            ParseError::InvalidUriSyntax, 
            ReplyStatus::BadRequest,
            "Malformed request: Userinfo detected."    
        );
	}
	return true;
}

bool RequestParser::pathTooLong(const std::string& copy_uri)
{
	if (copy_uri.length() > 255)
	{
		m_state = ParserState::ERROR;
		throw HttpParseException(
            ParseError::UriTooLong, 
            ReplyStatus::UriTooLong,
            "Malformed request: Path too long."    
        );
	}
	return true;
}

bool RequestParser::errorOnEmpty(const std::string& copy_uri)
{
	if (copy_uri.empty() == true)
	{
		m_state = ParserState::ERROR;
		throw HttpParseException(
            ParseError::InvalidUriSyntax, 
            ReplyStatus::BadRequest,
            "Malformed request: Uri cannot be empty."    
        );
	}
	return true;
}
void RequestParser::trimFragment(std::string& copy_uri)
{
	size_t pos;

	pos = copy_uri.find('#');
	if (pos == copy_uri.npos)
		return;
	copy_uri = copy_uri.substr(0, pos);
}

bool RequestParser::storeQuery(std::string& copy_uri)
{
	size_t pos;

	pos = copy_uri.find('?');
	if (pos == copy_uri.npos)
		return true;
	m_request.setQuery(copy_uri.substr(pos + 1)); 
	copy_uri.erase(pos, std::string::npos);
	return true;
}

bool RequestParser::validateLeadingSlash(const std::string& copy_uri)
{
	if (copy_uri.front() != '/')
	{
		m_state = ParserState::ERROR;
		throw HttpParseException(
            ParseError::InvalidUriSyntax, 
            ReplyStatus::BadRequest,
            "Malformed request: No leading slash found."    
        );
	}
	return true;
}

bool RequestParser::rejectNullBytes(std::string& parsed_uri)
{
	if (parsed_uri.find("%00") != std::string::npos || parsed_uri.find('\0') != std::string::npos)
	{
		m_state = ParserState::ERROR;
		throw HttpParseException(
            ParseError::InvalidUriSyntax, 
            ReplyStatus::BadRequest,
            "Malformed request: Nullbyte found in path."    
        );
	}
	return true;
}

static char hexToDecimal(char c)
{
	unsigned char uc = static_cast<unsigned char>(c);
	if (uc >= '0' && uc <= '9')
		return (uc - '0');
	if (uc >= 'A' && uc <= 'F')
		return (uc - 'A' + 10);
	if (uc >= 'a' && uc <= 'f')
		return (uc - 'a' + 10);
	return (0);
}

char RequestParser::decodeByte(char c1, char c2)
{
	unsigned char decoded_char = 0;
	decoded_char += hexToDecimal(c1);
	decoded_char *= 16;
	decoded_char += hexToDecimal(c2);
	return decoded_char;
}

/*
    Using string_view so this table can be built at compile time
    without extra lifetime/ownership weirdness.

    [](){...} is an anonymous lambda, and the final (); runs it immediately.
    It returns a fully initialized std::array<bool, 256> for safe_decode_lookup.

    Also: because this is static at file scope, it stays local to this .cpp file only.
*/
constexpr static std::array<bool, 256> safe_decode_lookup = []()
{
	std::array<bool, 256> result{};
	const std::string_view safe_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
	for (uint8_t i : safe_chars)
	{
		result[i] = true;
	}
	return result;
}();


static bool check_safe_decode(char c)
{
	return safe_decode_lookup[static_cast<unsigned char>(c)];
}

bool RequestParser::validateHexBytes(std::string& parsed_uri)
{
	for (size_t i = 0; i < parsed_uri.length(); i++)
	{
		if (parsed_uri[i] == '%')
		{
			if (static_cast<size_t>(i+2) >= parsed_uri.length())
			{
				m_state = ParserState::ERROR;
				throw HttpParseException(
        		    ParseError::InvalidUriSyntax, 
        		    ReplyStatus::BadRequest,
        		    "Malformed request: Hexbyte intersects with end of uri."    
        		);
			}
			if (!isxdigit(static_cast<unsigned char>(parsed_uri[i+1])) || !isxdigit(static_cast<unsigned char>(parsed_uri[i+2])))
			{
				m_state = ParserState::ERROR;
				throw HttpParseException(
        		    ParseError::InvalidUriSyntax, 
        		    ReplyStatus::BadRequest,
        		    "Malformed request: Hexbyte improperly formed."    
        		);
			}
			char decoded_char = decodeByte(parsed_uri[i+1], parsed_uri[i+2]);
			if (check_safe_decode(decoded_char))
				parsed_uri.replace(i, 3, 1, decoded_char);
			else
				i += 2;
		}
	}
	return true;
}

bool RequestParser::normalizePath(std::string& parsed_uri)
{
	std::vector<std::string> segments;
	size_t start = 1;

	for (size_t pos = 1; pos <= parsed_uri.length(); pos++)
	{
		if (pos == parsed_uri.length() || parsed_uri[pos] == '/')
		{
			std::string segment = parsed_uri.substr(start, pos - start);
			if (segment != "." && segment != ".." && !segment.empty())
				segments.push_back(segment);
			if (segment == "..")
			{
				if (!segments.empty())
					segments.pop_back();
			}
			start = pos + 1;
		}
	}
	parsed_uri = "/";
	for (size_t i = 0; i < segments.size(); i++)
	{
		parsed_uri += segments[i];
		if (i < segments.size() - 1)
			parsed_uri += "/";
	}
	return true;
}

bool RequestParser::normalizeURI(std::string& parsed_uri)
{
	if (!rejectNullBytes(parsed_uri))
		return false;
	if (!validateHexBytes(parsed_uri))
		return false;
	if (!normalizePath(parsed_uri))
		return false;
	// std::cout << "NORMALIZED STRING: " << parsed_uri << std::endl;
	return true;
}

bool RequestParser::parseURI(void)
{
	std::string working_uri(m_request.getRawUri());
	if (!errorOnEmpty(working_uri))
		return false;
	if (!validateLeadingSlash(working_uri))
		return false;
	if (!pathTooLong(working_uri))
		return false;
	if (!errorOnScheme(working_uri))
		return false;
	if (!errorOnAuthority(working_uri))
		return false;
	if (!errorOnUserInfo(working_uri))
		return false;
	trimFragment(working_uri);
	if (!storeQuery(working_uri))
		return false;
	if (!normalizeURI(working_uri))
		return false;
	m_request.setPath(working_uri);
	return true;
}

