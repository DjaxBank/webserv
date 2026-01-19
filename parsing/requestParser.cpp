#include "../inc/requestParser.hpp"
#include <sstream>
#include <iostream>
#include "../inc/requestParser.hpp"
#include "../inc/Request.hpp"

RequestParser::RequestParser() : m_buffer() {}

RequestParser::RequestParser(const RequestParser& other) : m_buffer(other.m_buffer) {}

RequestParser& RequestParser::operator=(const RequestParser& other) {
    if (this != &other) {
        m_buffer = other.m_buffer;
    }
    return *this;
}

RequestParser::~RequestParser() {}

std::string method_tostring(HttpMethod method)
{
    switch(method)
    {
        case HttpMethod::GET: return "GET";
        case HttpMethod::HEAD: return "HEAD";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::OPTIONS: return "OPTIONS";
        case HttpMethod::NONE: return "NONE";
    }
    return "NONE";
}

HttpMethod string_tomethod(const std::string& str)
{
	if (str == "GET") return HttpMethod::GET;
	if (str == "HEAD") return HttpMethod::HEAD;
	if (str == "POST") return HttpMethod::POST;
	if (str == "PUT") return HttpMethod::PUT;
	if (str == "DELETE") return HttpMethod::DELETE;
	if (str == "OPTIONS") return HttpMethod::OPTIONS;
	return HttpMethod::NONE;
}

// this will be put in request handling later
// but is here as an example
void handle_method(HttpMethod method)
{
	switch (method)
	{
		case HttpMethod::GET:
			// implement
			break; 
		case HttpMethod::HEAD: 
			// implement
			break; 
		case HttpMethod::POST:
			// implement
			break; 
		case HttpMethod::PUT:
			// implement
			break; 
		case HttpMethod::DELETE:
			// implement
			break; 
		case HttpMethod::OPTIONS:
			// implement
			break; 
		case HttpMethod::NONE:
    	   // handle error/invalid
    	   break;
	}
}

HttpMethod RequestParser::getMethod(const std::string &str) const
{
	return (string_tomethod(str));
}

bool RequestParser::parseLine(Request& request_data, const std::string& line)
{
	// hack solution for testing to add \n because getline strips it.
	m_buffer += line + '\n';
	std::cout << "Buffer: " << line << std::endl;
	size_t pos = m_buffer.find("\r\n");
	if (pos == std::string::npos)
	{
		std::cout << "found nothing\n";
		return false;
	}
	else
	{
		pos = m_buffer.find(' ');
		HttpMethod method = getMethod(m_buffer.substr(0, pos));
		std::cout << "Found method: " << method_tostring(method) << std::endl;
		if (method == HttpMethod::NONE)
			return false;
		request_data.setMethod(method_tostring(method));
		return true;
	}
}

int main() {
	// construct a request object to populate with data
	Request request_data;
	RequestParser parser;

    // example HTTP request as a str
    std::string mock_request = "GET /index.html HTTP/1.1\r\n"
                                "Host: example.com\r\n"
                                "Connection: close\r\n"
                                "\r\n";
    
    // Wrap it in a stream
    std::istringstream input(mock_request);
    
    std::string line;
    
    // Read lines until empty (replace with socket later)
    while (std::getline(input, line)) {
        if (line.empty()) {
            std::cout << "Empty line (end of headers)\n";
            break;
        }
        std::cout << "Got line: " << line << "\n";
		std::cout << "Parsing\n";
		if (!parser.parseLine(request_data, line))
		{
			std::cout << "Error in parsing\n";
		}
    }
    
    return 0;
}