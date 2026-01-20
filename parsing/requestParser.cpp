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

    // extract method
    pos = request_line.find(' ');
    if (pos == std::string::npos)
        return setErrorAndReturn("no space after method", request_line);
    std::string method_token = request_line.substr(0, pos);
    HttpMethod method = getMethod(method_token);
    if (method == HttpMethod::NONE)
        return setErrorAndReturn("invalid method", request_line);
    m_request.setMethod(method_tostring(method));
    request_line = request_line.substr(pos + 1);

    // extract target
    pos = request_line.find(' ');
    if (pos == std::string::npos)
        return setErrorAndReturn("no space after target", request_line);
    std::string target_token = request_line.substr(0, pos);
    if (target_token.empty())
        return setErrorAndReturn("empty target", request_line);
    m_request.setTarget(target_token);
    request_line = request_line.substr(pos + 1);

    // extract version
	std::string version_token = request_line;
    if (version_token.empty() || version_token.find(' ') != std::string::npos)
        return setErrorAndReturn("version missing or has spaces", version_token);
    if (!validateHTTPVersion(version_token))
        return setErrorAndReturn("invalid HTTP version", version_token);
    m_request.setVersion(version_token);

	// no errors. ready for next state of parsing
    m_state = ParserState::HEADERS;
}

/* Extracts header key. Errors return empty string*/
std::string RequestParser::extractKey(const std::string& header_token)
{
    size_t pos = header_token.find(':');
    if (pos == std::string::npos)
        return "";
    
    std::string key = header_token.substr(0, pos);
    if (key.find(' ') != std::string::npos || key.find('\t') != std::string::npos)
        return "";
    return key;
}

/* Extracts header value for key pair. Values allowed to be empty*/
std::string RequestParser::extractValue(const std::string& header_token)
{
    size_t pos = header_token.find(':');
    if (pos == std::string::npos)
        return "";
   std::string value = header_token.substr(pos + 1);
    return trimValue(value);
}
/* Trims header values of leading/trailing whitespace */
std::string RequestParser::trimValue(const std::string& value)
{
    std::string result = value;
    
    size_t start = result.find_first_not_of(" \t");
    if (start != std::string::npos)
        result = result.substr(start);
    else
        return "";
    
    size_t end = result.find_last_not_of(" \t");
    if (end != std::string::npos)
        result = result.substr(0, end + 1);
    
    return result;
}


/* Parses header of HTTP request and adds them to a request's key:value map */
void RequestParser::parseHeaders()
{
    const size_t MAX_HEADER_SIZE = 32768;
    // Expected headers are like "Host: localhost:8080\r\n"
    while (true)
    {
        size_t pos = m_buffer.find("\r\n");
        if (pos == std::string::npos)
            return;
        if (pos > MAX_HEADER_SIZE)
            return setErrorAndReturn("header line too long", "");
        std::string header_token = m_buffer.substr(0, pos);
        m_buffer.erase(0, pos + 2);
        if (header_token.empty()) {
            HttpMethod method = string_tomethod(m_request.getMethod());
            if (method == HttpMethod::GET || method == HttpMethod::HEAD || method == HttpMethod::DELETE)
                m_state = ParserState::COMPLETE;
            else
                m_state = ParserState::BODY;
            return;
        }
        
        std::string key = extractKey(header_token);
        if (key.empty())
            return setErrorAndReturn("invalid header key", header_token);

        std::string value = extractValue(header_token);

        m_request.addHeader(key, value);
    }
    // to do
    // validate required headers if HTTP version == 1.1
    // check based on method if I expect a body or not and change state based on that
    // also based on method I may need content length and transfer encoding i think
}

/* Needs implementing */
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
    m_buffer += data;
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
            << " \nheaders=" << m_request.getHeaders().size() << "\n";
            
    const std::map<std::string, std::string>& headers = m_request.getHeaders();
    for (const auto& pair : headers)
    {
        std::cerr << "  [" << pair.first << "]: " << pair.second << "\n";
    }
    
    std::cerr << " \nbody_bytes=" << m_request.getBody().size()
            << " \nbuffer_prefix=\"" << m_buffer.substr(0, 80) << "\""
            << "\n------------------------------------\n";
}


/* Searchers for key in headers and returns value as a string
    @param key for the value you are trying to find
    e.g. "Host"
*/
std::string RequestParser::getHeader(const std::string& key) const
{
    const auto& headers = m_request.getHeaders();
    auto iterator = headers.find(key);
    if (iterator != headers.end())
    {
        return iterator->second;
    }
    else
        return "";
    
}

int main() {
    RequestParser parser;

    std::string mock_request = "GET /index.html HTTP/1.1\r\n"
                                "Host: localhost:8080\r\n"
                                "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36\r\n"
                                "Accept:: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"
                                "Accept-Language: en-US,en;q=0.9\r\n"
                                "Accept-Encoding: gzip, deflate\r\n"
                                "Connection: keep-alive\r\n"
                                "Upgrade-Insecure-Requests: 1\r\n"
                                "\r\n";
    
    // AI generated test case to simulate socket feeding data.
    // just wanted a quick check to make sure what I'm doing is portable

    // Simulate socket recv() by feeding data in chunks
    // Real socket: recv(socket_fd, buffer, 1024, 0)
    
    size_t chunk_size = 50;  // Simulate small socket recv chunks
    size_t offset = 0;
    
    while (offset < mock_request.size())
    {
        // Simulate recv() call - get chunk of data
        size_t bytes_to_read = std::min(chunk_size, mock_request.size() - offset);
        std::string chunk = mock_request.substr(offset, bytes_to_read);
        offset += bytes_to_read;
        
        std::cout << ">> Recv chunk [" << bytes_to_read << " bytes]: " 
                  << chunk.substr(0, 30) << (chunk.size() > 30 ? "..." : "") << "\n";
        
        // Feed chunk to parser
        if (!parser.fetch_data(chunk))
        {
            std::cerr << "Parse failed\n";
            return 1;
        }
        
        // Check if parsing is complete
        if (parser.getState() == ParserState::COMPLETE)
        {
            std::cout << "Parsing complete!\n";
            break;
        }
        else if (parser.getState() == ParserState::ERROR)
        {
            std::cerr << "Parse error\n";
            return 1;
        }
    }
    
    if (parser.getState() == ParserState::COMPLETE)
    {
        std::cout << "Parsed successfully\n";
        parser.debugState();
    }
    else
    {
        std::cerr << "Parsing incomplete\n";
        return 1;
    }
    
    return 0;
}