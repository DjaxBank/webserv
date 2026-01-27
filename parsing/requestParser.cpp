#include "../inc/requestParser.hpp"
#include <sstream>
#include <iostream>
#include "../inc/Request.hpp"
#include <cctype>

// Important to mitigate DOS attacks
// Max size of a single header line must be less than 32kb
const size_t RequestParser::MAX_HEADER_SIZE = 32768;
// Max size of headers must be less than 256kb
const size_t RequestParser::MAX_TOTAL_HEADER_SIZE = 262144;

// Max size of body must be 100MB or less
const size_t RequestParser::MAX_BODY_SIZE = 100 * 1024 * 1024;
// Max size of chunks must be 8MB or less
const size_t RequestParser::MAX_CHUNK_SIZE = 8 * 1024 * 1024;

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
const std::string RequestParser::getHeader(const std::string& key) const
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
        std::string encoding = trimValue(t_encoding);

        if (encoding == "chunked")
        {
            m_request.setChunked(true);
            size_t pos = m_buffer.find("\r\n\r\n");
            if (pos == std::string::npos)
                return fail("malformed body seperation line", "");
            m_buffer.erase(0, pos + 4);
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
        size_t pos = m_buffer.find("\r\n\r\n");
            if (pos == std::string::npos)
                return fail("malformed body seperation line", "");
        m_buffer.erase(0, pos + 4);
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
        size_t pos = m_buffer.find("\r\n\r\n");
        if (pos == 0)
        {
            break;
        }
        pos = m_buffer.find("\r\n");
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

// I probably need a function that grabs the next \r\n token

void RequestParser::parseBody()
{
    while(true)
    {
        if (m_request.getChunked() == true)
        {
            size_t pos = m_buffer.find("\r\n");
            if (pos == std::string::npos)
                return;
            
            if (m_need_chunk_size == true)
            {
                std::string hex_value = m_buffer.substr(0, pos);
                
                // Handle empty hex_value (shouldn't happen if buffer has \r\n)
                if (hex_value.empty())
                    return setErrorAndReturn("empty chunk size", hex_value);
                
                size_t chunk_size = 0;
                try
                {
                    chunk_size = std::stoul(hex_value, 0, 16);
                }
                catch (std::invalid_argument)
                {
                    return setErrorAndReturn("malformed chunk size", hex_value);
                }
                catch (std::out_of_range)
                {
                    return setErrorAndReturn("chunk size out of range", "");
                }
                if (chunk_size == 0)
                {
                    m_buffer.erase(0, 1);
                    if (m_buffer.size() < 2 || m_buffer.compare(0, 2, "\r\n") != 0)
                        return setErrorAndReturn("malformed chunked encoding end", m_buffer);
                    m_buffer.erase(0, 2);
                    m_state = ParserState::COMPLETE;
                    return;
                }
                
                if (chunk_size > MAX_CHUNK_SIZE)
                    return setErrorAndReturn("chunk size too large", "");
                if (m_chunkBytesReceived + chunk_size > MAX_BODY_SIZE)
                    return setErrorAndReturn("body too large", "");
                
                m_chunkBytesRemaining = chunk_size;
                m_buffer.erase(0, pos + 2);
                m_need_chunk_size = false;
            }

            if (m_buffer.size() < m_chunkBytesRemaining + 2)
                return;
            
            m_request.appendBody(m_buffer.substr(0, m_chunkBytesRemaining));
            m_chunkBytesReceived += m_chunkBytesRemaining;
            m_buffer.erase(0, m_chunkBytesRemaining);
            
            if (m_buffer.compare(0, 2, "\r\n") != 0)
                return setErrorAndReturn("chunk data and CRLF mismatch", m_buffer);
            
            m_buffer.erase(0, 2);
            m_need_chunk_size = true;
        }
        else
        {
            // TODO: implement content-length body parsing
            return;
        }
    }
}

/* Feeds data into the parser and processes based on current state
 * @param data: chunk of HTTP request data
 * @return: false if ERROR state reached, true otherwise
 * Use: call repeatedly with incoming socket data until parsing completes
 */
bool RequestParser::fetch_data(const std::string& data)
{
    m_buffer += data;
    
    while (m_state != ParserState::ERROR && m_state != ParserState::COMPLETE)
    {
        switch (m_state)
        {
            case ParserState::REQUEST_LINE:
                parseRequestLine();
                if (m_state == ParserState::REQUEST_LINE)
                    return true;
                break;
            case ParserState::HEADERS:
                parseHeaders();
                if (m_state == ParserState::HEADERS)
                    return true;
                break;
            case ParserState::BODY:
                parseBody();
                if (m_state == ParserState::BODY)
                    return true;
                break;
            case ParserState::ERROR:
                return false;
            case ParserState::COMPLETE:
                return true;
        }
    }
    
    return m_state != ParserState::ERROR;
}

/* Returns current parser state */
ParserState RequestParser::getState() const
{
    return m_state;
}

const HttpMethod& RequestParser::getMethod() const
{
    return m_request.getMethod();
}

const std::string& RequestParser::getTarget() const
{
    return m_request.getTarget();
}

const HttpVersion& RequestParser::getVersion() const
{
    return m_request.getVersion();
}

const std::vector<uint8_t>& RequestParser::getBody() const
{
    return m_request.getBody();
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

// Main function removed - use debug_parser.cpp for testing