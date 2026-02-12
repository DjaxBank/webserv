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
	size_t pos;

	pos = copy_uri.find(HTTP_CONSTANT::AUTHORITY_PREFIX);
	if (pos != std::string::npos)
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

bool RequestParser::decodeHexBytes(std::string& parsed_uri)
{
	// some looping logic until end of string
	
	// find %
	// is pos + 2 greater than the length of the string? if so error
	// check if pos + 1 and pos + 2 are digits
	// extract +2 and put in stoi, storing result.
	// pull out 3 chars and replace with value, correctly concatinating
}

bool RequestParser::normalizeURI(std::string& parsed_uri)
{
	if (!rejectNullBytes(parsed_uri))
		return false;
	if (!decodeHexBytes(parsed_uri))
	return true;
}

bool RequestParser::parseURI(void)
{
	std::string uri = "/example.com:443/products/item?id=123&category=books#details";

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

