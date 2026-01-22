#include "../inc/requestParser.hpp"
#include <sstream>
#include <iostream>
#include "../inc/Request.hpp"
#include <cctype>

// Important to mitigate DOS attacks
// Max size of a single header line must be less than 32kb
const size_t RequestParser::MAX_HEADER_SIZE = 32768;
// Make size of headers must be less than 256kb
const size_t RequestParser::MAX_TOTAL_HEADER_SIZE = 262144;

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
        case HttpMethod::POST: return "POST";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::NONE: return "NONE";
    }
    return "NONE";
}

/* Converts string to HttpMethod enum; returns NONE if invalid */
HttpMethod string_tomethod(const std::string& str)
{
    if (str == "GET") return HttpMethod::GET;
    if (str == "POST") return HttpMethod::POST;
    if (str == "DELETE") return HttpMethod::DELETE;
    return HttpMethod::NONE;
}
/* Converts string to HttpVersion enum; returns NONE if invalid */
HttpVersion string_toversion(const std::string &version)
{
    if (version == "HTTP/1.0") return HttpVersion::HTTP_1_0;
    if (version == "HTTP/1.1") return HttpVersion::HTTP_1_1;
    return HttpVersion::NONE;
}
/* Converts HttpVersion enum to string; returns "NONE" if invalid*/
std::string version_tostring(const HttpVersion &version)
{
    switch (version)
    {
        case HttpVersion::HTTP_1_0: return "HTTP/1.0";
        case HttpVersion::HTTP_1_1: return "HTTP/1.1";
        case HttpVersion::NONE: return "NONE";
    }
    return "NONE";
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
        case HttpMethod::POST:
            // implement
            break; 
            // implement
            break; 
        case HttpMethod::DELETE:
            // implement
            break; 
            // implement
            break; 
        case HttpMethod::NONE:
           // handle error/invalid
           break;
    }
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

/* Wrapper for parsing failures */
bool RequestParser::fail(const char* reason, const std::string& line)
{
    setErrorAndReturn(reason, line);
    return false;
}

/* Validates HTTP version string against supported versions
 * Currently accepts: HTTP/1.0, HTTP/1.1
 */
bool RequestParser::validateHTTPVersion(const std::string& version)
{
    return version == "HTTP/1.0"
        || version == "HTTP/1.1";
}

// CONSIDER REFACTORING
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
    HttpMethod method = string_tomethod(method_token);
    if (method == HttpMethod::NONE)
        return setErrorAndReturn("invalid method", request_line);
    m_request.setMethod(method);
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
    m_request.setVersion(string_toversion(version_token));

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

/* returns a header as a string based on a key value
    @param key will return the value based on they or "" if key not found*/
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

/* Helper function to validate content-lenght header
    @param value is the value of the content-length key pair
    @param out_length a size_t value to store the content length in after parsing */
bool RequestParser::validateContentLength(const std::string& value, size_t& out_length)
{
    size_t one_hundread_MB = 104857600;

    if (value.empty())
        return false;
    for (unsigned char c : value)
    {
        if (!std::isdigit(c))
            return false;
    }

    std::stringstream sstream(value);
    if (!(sstream >> out_length))
        return false;
    if (!sstream.eof())
        return false;
    if (out_length > one_hundread_MB)
        return false;
    return true;
}
/* Enforces HTTP/1.1 requiring the "Host" header */
bool RequestParser::validateRequiredHeaders()
{
    if (m_request.getVersion() == HttpVersion::HTTP_1_1 && getHeader("Host").empty())
        return fail("missing Host header", "");
    return true;
}
/* Parses metadata relating to body informaiton, determins if body is present
    and its related headers are valid */
bool RequestParser::parseBodyMetadata()
{
    const std::string t_encoding = getHeader("Transfer-Encoding");
    const std::string c_length = getHeader("Content-Length");

    if (!t_encoding.empty() && !c_length.empty())
        return fail("transfer encoding and content length present in header", "");

    if (!t_encoding.empty())
    {
        if (t_encoding.find("chunked") != std::string::npos)
        {
            m_request.setChunked(true);
            m_state = ParserState::BODY;
            return true;
        }
        return fail("unsupported transfer-encoding method", "");
    }

    if (!c_length.empty())
    {
        size_t content_len;
        if (!validateContentLength(c_length, content_len))
            return fail("malformed content-length", "");
        m_request.setContentLen(content_len);
        m_state = ParserState::BODY;
        return true;
    }

    return true;
}

/* Parses header of HTTP request and adds them to a request's key:value map */
void RequestParser::parseHeaders()
{
    size_t total_header_size = 0;
    while (true)
    {
        size_t pos = m_buffer.find("\r\n");
        if (pos == std::string::npos)
            return;
        if (pos > MAX_HEADER_SIZE)
            return setErrorAndReturn("header line too long", "");
        total_header_size += pos + 2;
        if (total_header_size > MAX_TOTAL_HEADER_SIZE)
            return setErrorAndReturn("total header file size too large", "");
        std::string header_token = m_buffer.substr(0, pos);
        m_buffer.erase(0, pos + 2);
        if (header_token.empty()) {
            break;
        }
        
        std::string key = extractKey(header_token);
        if (key.empty())
            return setErrorAndReturn("invalid header key", header_token);

        std::string value = extractValue(header_token);

        m_request.addHeader(key, value);
    }

    if (!validateRequiredHeaders())
        return;
    if (!parseBodyMetadata())
        return;

    if (m_state != ParserState::BODY)
        m_state = ParserState::COMPLETE;
    
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
            << " \nmethod=\"" << method_tostring(m_request.getMethod()) << "\""
            << " \ntarget=\"" << m_request.getTarget() << "\""
            << " \nversion=\"" << version_tostring(m_request.getVersion()) << "\""
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

int main() {
    RequestParser parser;

    std::string mock_request = "GET /index.html HTTP/1.1\r\n"
                                "Host: localhost:8080\r\n"
                                "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36\r\n"
                                "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"
                                "Accept-Language: en-US,en;q=0.9\r\n"
                                "Accept-Encoding: gzip, deflate\r\n"
                                "Connection: keep-alive\r\n"
                                "Upgrade-Insecure-Requests: 1\r\n"
                                "Content-Length: 500\r\n"
                                "Help: lol\r\n"
                                "\r\n"
                                "\r\n"
                                "\r\n"
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
        
    }
 
    if (parser.getState() == ParserState::COMPLETE)
    {
        std::cout << "Parsed successfully\n";
        parser.debugState();
        return 0;
    }
    else
    {   
        std::cerr << "Parsing incomplete\n";
        parser.debugState();
        return 1;
    }
}