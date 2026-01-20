#include "../inc/requestParser.hpp"
#include <sstream>
#include <iostream>
#include "../inc/requestParser.hpp"
#include "../inc/Request.hpp"

RequestParser::RequestParser() : m_buffer(), m_state(ParserState::REQUEST_LINE), m_request() {}

RequestParser::RequestParser(const RequestParser& other)
    : m_buffer(other.m_buffer), m_state(other.m_state), m_request(other.m_request) {}

RequestParser& RequestParser::operator=(const RequestParser& other) {
    if (this != &other) {
        m_buffer = other.m_buffer;
        m_state   = other.m_state;
        m_request = other.m_request;
    }
    return *this;
}

RequestParser::~RequestParser() {}

/* Converts HttpMethod enum to string  */
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

/* Converts string to HttpMethod enum; returns NONE if invalid */
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

/* Converts ParserState enum to string */
std::string state_tostring(ParserState state)
{
    switch(state)
    {
        case ParserState::REQUEST_LINE: return "REQUEST_LINE";
        case ParserState::HEADERS: return "HEADERS";
        case ParserState::BODY: return "BODY";
        case ParserState::COMPLETE: return "COMPLETE";
        case ParserState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

/* placeholder, needs implementing */
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

/* Wrapper to convert string to HttpMethod enum */
HttpMethod RequestParser::getMethod(const std::string &str) const
{
    return (string_tomethod(str));
}

/* Sets parser state to ERROR and logs debug info
 * @param reason: optional error description
 * @param line: the problematic line being parsed
 */
void RequestParser::setErrorAndReturn(const char* reason, const std::string& line)
{
    std::cerr << "[ParserError] state=" << static_cast<int>(m_state)
              << " reason=\"" << (reason ? reason : "") << "\""
              << " line=\"" << line << "\""
              << " buffer_prefix=\"" << m_buffer.substr(0, 80) << "\""
              << std::endl;
    m_state = ParserState::ERROR;
}

/* Validates HTTP version string against supported versions
 * Currently accepts: HTTP/1.0, HTTP/1.1, HTTP/2.0, HTTP/3.0
 */
bool RequestParser::validateHTTPVersion(const std::string& version)
{
    return version == "HTTP/1.0"
        || version == "HTTP/1.1"
        || version == "HTTP/2.0"
        || version == "HTTP/3.0";
}

/* Parses the HTTP request line: METHOD TARGET VERSION
 * Extracts and validates each component, then transitions state
 * Errors if any component is missing, empty, or invalid
 */
void RequestParser::parseRequestLine()
{
    size_t pos = m_buffer.find("\r\n");
    if (pos == std::string::npos)
        return;

    std::string request_line = m_buffer.substr(0, pos);
    m_buffer.erase(0, pos + 2);

    // method
    pos = request_line.find(' ');
    if (pos == std::string::npos)
        return setErrorAndReturn("no space after method", request_line);
    std::string method_token = request_line.substr(0, pos);
    HttpMethod method = getMethod(method_token);
    if (method == HttpMethod::NONE)
        return setErrorAndReturn("invalid method", request_line);
    m_request.setMethod(method_tostring(method));
    request_line = request_line.substr(pos + 1);

    // target
    pos = request_line.find(' ');
    if (pos == std::string::npos)
        return setErrorAndReturn("no space after target", request_line);
    std::string target = request_line.substr(0, pos);
    if (target.empty())
        return setErrorAndReturn("empty target", request_line);
    m_request.setTarget(target);
    request_line = request_line.substr(pos + 1);

    // version
    if (request_line.empty() || request_line.find(' ') != std::string::npos)
        return setErrorAndReturn("version missing or has spaces", request_line);
    if (!validateHTTPVersion(request_line))
        return setErrorAndReturn("invalid HTTP version", request_line);
    m_request.setVersion(request_line);

	// ready for next state of parsing
    m_state = ParserState::HEADERS;
}

/* needs implementing */
void RequestParser::parseHeaders()
{
    std::cout << "IN HEADERS: " << m_buffer << std::endl;
    return;
}

/* Parses HTTP body (stub implementation) */
void RequestParser::parseBody()
{
    //implement
    return;
}

/* Feeds data into the parser and processes based on current state
 * @param data: chunk of HTTP request data
 * @return: false if ERROR state reached, true otherwise
 * Use: call repeatedly with incoming socket data until parsing completes
 * 
 *
 * ***CURRENTLY A WORK AROUND, SOCKET DATA NOT IMPLEMENTED***
 * *** check main in same file for workaround ***
 */
bool RequestParser::fetch_data(const std::string& data)
{
    // m_buffer += data;
    // TEMP hack solution for testing to add \n because getline strips it.
    m_buffer += data + '\n';
    switch (m_state)
    {
        case ParserState::REQUEST_LINE:
            parseRequestLine();
            break;
        case ParserState::HEADERS:
            parseHeaders();
            break;
        case ParserState::BODY:
            parseBody();
            break;
        case ParserState::ERROR:
            return false;
        case ParserState::COMPLETE:
            return true;
    }
    return m_state != ParserState::ERROR;
}

/* Returns current parser state */
ParserState RequestParser::getState() const
{
    return m_state;
}

/* Prints detailed parser state and request contents for debugging
 * @param label: optional label to identify the debug point
 */
void RequestParser::debugState(const char* label) const
{
    std::cerr << "\n------------------------------------"
            << "\n[ParserState] " << (label ? label : "")
            << " \nstate=" << state_tostring(m_state) << "\""
            << " \nmethod=\"" << m_request.getMethod() << "\""
            << " \ntarget=\"" << m_request.getTarget() << "\""
            << " \nversion=\"" << m_request.getVersion() << "\""
            << " \nheaders=" << m_request.getHeaders().size()
            << " \nbody_bytes=" << m_request.getBody().size()
            << " \nbuffer_prefix=\"" << m_buffer.substr(0, 80) << "\""
            << "\n------------------------------------\n";
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
        if (!parser.fetch_data(line))
        {
            return(1);
        }
    }
	std::cout << "Parsed successfully\n";
    parser.debugState();
    return 0;
}