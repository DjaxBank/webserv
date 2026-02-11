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

// std::string uriParser(const std::string& raw_uri)
// {
// 	return
// }

bool RequestParser::parseURI(void)
{
	std::string uri = "https://example.com:443/products/item?id=123&category=books#details";

	// will be m_request.getPath()
	std::string parsed_uri(uri);
	std::cout << "Starting parsing on: " << uri << std::endl;
	if (!errorOnScheme(parsed_uri))
		return false;
	std::cout << "Post skipScheme: " << uri << std::endl;
	return (true);
}