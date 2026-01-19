#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include <string>
#include "../inc/Request.hpp"

enum class HttpMethod
{
	GET,
	HEAD,
	POST,
	PUT,
	DELETE,
	OPTIONS,
	NONE,
};

std::string method_tostring(HttpMethod method);
HttpMethod string_tomethod(const std::string& str);

// this will be put in request handling later
// implementation example in requestParser.cpp
void handle_method(HttpMethod method);

class RequestParser
{
	private:
		// std::string m_line;
		std::string m_buffer;
	public:
		RequestParser();
		RequestParser(const RequestParser& other);
		RequestParser& operator=(const RequestParser& other);
		~RequestParser();
		HttpMethod getMethod(const std::string &str) const;
		bool parseLine(Request& request_data, const std::string& line);



};

#endif