#include <string>
#include <iostream>
#include <requestParser.hpp>
#include <Request.hpp>


bool RequestParser::errorOnScheme(const std::string& copy_uri)
{
	size_t pos;

	pos = copy_uri.find(HTTP_CONSTANT::SCHEME_SUFFIX);
	if (pos != std::string::npos)
	{
		setErrorAndReturn("malformed request: scheme detected", copy_uri);
		m_state = ParserState::ERROR;
		return false;
	}
	return true;
}

bool RequestParser::errorOnAuthority(const std::string& copy_uri)
{
	if (copy_uri.compare(0, 2, "//") == 0)
	{
		setErrorAndReturn("malformed request: authority detected/incorrect file path", copy_uri);
		m_state = ParserState::ERROR;
		return false;
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
		setErrorAndReturn("malformed request: userinfo detected", copy_uri);
		m_state = ParserState::ERROR;
		return false;
	}
	return true;
}

bool RequestParser::pathTooLong(const std::string& copy_uri)
{
	if (copy_uri.length() > 255)
	{
		setErrorAndReturn("malformed request: path too long (>255)", copy_uri);
		m_state = ParserState::ERROR;
		return false;
	}
	return true;
}

bool RequestParser::errorOnEmpty(const std::string& copy_uri)
{
	if (copy_uri.empty() == true)
	{
		setErrorAndReturn("malformed request: uri cannot be empty", copy_uri);
		m_state = ParserState::ERROR;
		return false;
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
		setErrorAndReturn("malformed request: no leading slash found", copy_uri);
		m_state = ParserState::ERROR;
		return false;
	}
	return true;
}

bool RequestParser::rejectNullBytes(std::string& parsed_uri)
{
	if (parsed_uri.find("%00") != std::string::npos || parsed_uri.find('\0') != std::string::npos)
	{
		setErrorAndReturn("malformed request: nullbyte found in path", parsed_uri);
		m_state = ParserState::ERROR;
		return false;
	}
	return true;
}

static char hexToDecimal(char c)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	else
		return ((c - 'A') + 10);
}

char RequestParser::decodeByte(char c1, char c2)
{
	unsigned char decoded_char = 0;
	decoded_char += hexToDecimal(c1);
	decoded_char *= 16;
	decoded_char += hexToDecimal(c2);
	return decoded_char;
}

bool RequestParser::validateHexBytes(std::string& parsed_uri)
{
	for (size_t i = 0; i < parsed_uri.length(); i++)
	{
		if (parsed_uri[i] == '%')
		{
			if (static_cast<size_t>(i+2) > parsed_uri.length())
			{
				setErrorAndReturn("malformed request: hexbyte intersects with end of uri", parsed_uri);
				m_state = ParserState::ERROR;
				return false;
			}
			if (!isxdigit(parsed_uri[i+1]) || !isxdigit(parsed_uri[i+2]))
			{
				setErrorAndReturn("malformed request: hexbyte improperly formed", parsed_uri);
				m_state = ParserState::ERROR;
				return false;
			}
			if (parsed_uri[i+1] < 'A' )
			parsed_uri.replace(i, 3, 1, decodeByte(parsed_uri[i+1], parsed_uri[i+2]));

		}
	}
	return true;
}

bool RequestParser::normalizePath(std::string& parsed_uri)
{
	std::string input_buffer = parsed_uri;
	std::string output_buffer;
	if (input_buffer[0] == '/')
		output_buffer.insert(0, "/");
	input_buffer.erase(0, 1);
	while (!input_buffer.empty())
	{
		if (input_buffer.compare(0, 2, "./") == 0)
			input_buffer.erase(0, 2);
		else if (input_buffer.compare(0, 3, "../") == 0)
			input_buffer.erase(0, 3);
		else if (input_buffer.compare(0, 3, "/./") == 0)
		{
			input_buffer.erase(0, 3);
			input_buffer.insert(0, "/");
		}
		else if (input_buffer.compare(0, 4, "/../") == 0)
		{
			size_t pos = output_buffer.rfind('/');
			if (pos != std::string::npos)
			{
				output_buffer.erase(pos);
			}
			input_buffer.erase(0, 4);
			input_buffer.insert(0, "/");
		}
		else if (input_buffer.compare(0, 3, "/..") == 0)
		{
			if (input_buffer.length() == 3 || input_buffer[3] == '/')
			{
				size_t pos = output_buffer.rfind('/');
				if (pos != std::string::npos)
				{
					output_buffer.erase(pos);
				}
				input_buffer.erase(0, 3);
				input_buffer.insert(0, "/");
			}
		}
		else if (input_buffer.compare(0, 2, "/.") == 0)
		{
			if (input_buffer.length() == 2 || input_buffer[2] == '/')
			{
				input_buffer.erase(0, 2);
				input_buffer.insert(0, "/");
			}
		}
		else if (input_buffer == "." || input_buffer == "..")
		{
			input_buffer.erase(0);
		}
		else
		{
			size_t segment_end = input_buffer.find('/', 1);
			size_t segment_len;
			if (segment_end == std::string::npos)
			{
				segment_len = input_buffer.length();
				output_buffer += input_buffer.substr(0, segment_len);
				input_buffer.erase(0, segment_len);
			}
			else
			{
				output_buffer += input_buffer.substr(0, segment_end);
				input_buffer.erase(0, segment_end);
			}
		}	
	}
	parsed_uri = output_buffer;
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
	std::string uri = "/a//b";

	std::string working_uri(uri);
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
	return true;
}

