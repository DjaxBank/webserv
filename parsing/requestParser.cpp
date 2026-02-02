#include "../inc/requestParser.hpp"
#include <sstream>
#include <iostream>
#include "../inc/Request.hpp"
#include <cctype>

// ============================================================================
// CONSTANTS & DEFINITIONS
// ============================================================================

// Important to mitigate DOS attacks
// Max size of a single header line must be less than 32kb
const size_t RequestParser::MAX_HEADER_SIZE = 32768;
// Max size of headers must be less than 256kb
const size_t RequestParser::MAX_TOTAL_HEADER_SIZE = 262144;

// Max size of body must be 100MB or less
const size_t RequestParser::MAX_BODY_SIZE = 100 * 1024 * 1024;
// Max size of chunks must be 8MB or less
const size_t RequestParser::MAX_CHUNK_SIZE = 8 * 1024 * 1024;

// Other constant definitions

// length of "\r\n" aka for HTTP: CRLF - (Carriage Return, Line Feed)
const size_t CRLF_LENGTH = 2;
const std::string CRLF = "\r\n";
// length of "\r\n\r\n" aka for HTTP: Empty Line or Blank Line
const size_t EMPTY_LINE_LENGTH = 4;
const std::string EMPTY_LINE = "\r\n\r\n";

// ============================================================================
// CONSTRUCTORS & DESTRUCTOR
// ============================================================================

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

// ============================================================================
// GETTERS
// ============================================================================

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

// ============================================================================
// CONVERSION UTILITIES
// ============================================================================

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
        case HttpMethod::DELETE:
            // implement
            break; 
        case HttpMethod::NONE:
           // handle error/invalid
           break;
    }
}

// ============================================================================
// ERROR HANDLING
// ============================================================================

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

// ============================================================================
// STRING UTILITIES
// ============================================================================

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

/* Extracts next complete line (up to \r\n) from source string
 * Removes the extracted line + CRLF from source
 * @param source: string to extract from (modified in place)
 * @param out_token: extracted line without CRLF
 * @return: true if line found, false if incomplete
 */
bool RequestParser::extractLineToken(std::string& source, std::string& out_token)
{
    size_t line_end = source.find(CRLF);
    if (line_end == std::string::npos)
        return false;
    
    out_token = source.substr(0, line_end);
    source.erase(0, line_end + CRLF_LENGTH);
    return true;
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

// ============================================================================
// VALIDATION
// ============================================================================

/* Validates HTTP version string against supported versions
 * Currently accepts: HTTP/1.0, HTTP/1.1
 */
bool RequestParser::validateHTTPVersion(const std::string& version)
{
    return version == "HTTP/1.0"
        || version == "HTTP/1.1";
}

/* Helper function to validate content-length header
    @param value is the value of the content-length key pair
    @param out_length a size_t value to store the content length in after parsing */
bool RequestParser::validateContentLength(const std::string& value, size_t& out_length)
{
    if (value.empty())
        return false;

    for (unsigned char c : value)
    {
        if (!std::isdigit(c))
            return false;
    }

    try
    {
        out_length = std::stoul(value, nullptr, 10);
    }
    catch (const std::invalid_argument&)
    {
        return false;
    }
    catch (const std::out_of_range&)
    {
        return false;
    }
    
    if (out_length > MAX_BODY_SIZE)
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

// ============================================================================
// REQUEST LINE PARSING
// ============================================================================

/* Extracts and validates HTTP method from request line
 * Modifies request_line by removing method and following space
 * @return: true if valid method found, false otherwise
 */
bool RequestParser::extractMethod(std::string& request_line)
{
    size_t pos = request_line.find(' ');
    if (pos == std::string::npos)
    {
        setErrorAndReturn("no space after method", request_line);
        return false;
    }
    
    std::string method_token = request_line.substr(0, pos);
    HttpMethod method = string_tomethod(method_token);
    if (method == HttpMethod::NONE)
    {
        setErrorAndReturn("invalid method", request_line);
        return false;
    }
    
    m_request.setMethod(method);
    request_line = request_line.substr(pos + 1);
    return true;
}

/* Extracts and validates target URI from request line
 * Modifies request_line by removing target and following space
 * @return: true if valid target found, false otherwise
 */
bool RequestParser::extractTarget(std::string& request_line)
{
    size_t pos = request_line.find(' ');
    if (pos == std::string::npos)
    {
        setErrorAndReturn("no space after target", request_line);
        return false;
    }
    
    std::string target_token = request_line.substr(0, pos);
    if (target_token.empty())
    {
        setErrorAndReturn("empty target", request_line);
        return false;
    }
    
    m_request.setTarget(target_token);
    request_line = request_line.substr(pos + 1);
    return true;
}

/* Extracts and validates HTTP version from request line
 * @param version_token: the version string (should be last token)
 * @return: true if valid version, false otherwise
 */
bool RequestParser::extractVersion(const std::string& version_token)
{
    if (version_token.empty() || version_token.find(' ') != std::string::npos)
    {
        setErrorAndReturn("version missing or has spaces", version_token);
        return false;
    }
    
    if (!validateHTTPVersion(version_token))
    {
        setErrorAndReturn("invalid HTTP version", version_token);
        return false;
    }
    
    m_request.setVersion(string_toversion(version_token));
    return true;
}

/* Parses the HTTP request line: METHOD TARGET VERSION
 * Extracts and validates each component, then transitions state
 * Errors if any component is missing, empty, or invalid
 */
void RequestParser::parseRequestLine()
{
    std::string request_line;
    if (!extractLineToken(m_buffer, request_line))
        return;

    if (!extractMethod(request_line))
        return;
    
    if (!extractTarget(request_line))
        return;
    
    if (!extractVersion(request_line))
        return;

    m_state = ParserState::HEADERS;
}

// ============================================================================
// HEADER PARSING
// ============================================================================

// Needs a refactor into helper functions as well probably.
/* Extracts the complete headers section from buffer
 * @param out_headers_section: extracted headers without trailing \r\n\r\n
 * @return: true if complete section found, false if incomplete
 */
bool RequestParser::extractHeadersSection(std::string& out_headers_section)
{
    size_t header_end = m_buffer.find(EMPTY_LINE);
    if (header_end == std::string::npos)
        return false;
    
    if (header_end > MAX_TOTAL_HEADER_SIZE)
    {
        setErrorAndReturn("total header size too large", "");
        return false;
    }
    
    out_headers_section = m_buffer.substr(0, header_end);
    m_buffer.erase(0, header_end + EMPTY_LINE_LENGTH);
    return true;
}

/* Parses a single header line and adds to request
 * @param header_line: the line to parse (format: "Key: Value")
 * @return: true if valid and added, false on error
 */
bool RequestParser::parseHeaderLine(const std::string& header_line)
{
    if (header_line.empty())
        return true;
    
    if (header_line.length() > MAX_HEADER_SIZE)
    {
        setErrorAndReturn("header line too long", header_line);
        return false;
    }
    
    std::string key = extractKey(header_line);
    if (key.empty())
    {
        setErrorAndReturn("invalid header key", header_line);
        return false;
    }
    
    std::string value = extractValue(header_line);
    m_request.addHeader(key, value);
    return true;
}

/* Processes all header lines in the headers section
 * @param headers_section: complete headers text
 * @return: true if all headers parsed successfully
 */
bool RequestParser::processHeaderLines(const std::string& headers_section)
{
    size_t pos = 0;
    while (pos < headers_section.length())
    {
        size_t line_end = headers_section.find(CRLF, pos);
        if (line_end == std::string::npos)
        {
            std::string header_line = headers_section.substr(pos);
            if (!parseHeaderLine(header_line))
                return false;
            break;
        }
        
        std::string header_line = headers_section.substr(pos, line_end - pos);
        
        if (!parseHeaderLine(header_line))
            return false;
        
        pos = line_end + CRLF_LENGTH;
    }
    return true;
}

/* Parses metadata relating to body information, determines if body is present
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
    
    m_state = ParserState::COMPLETE;
    return true;
}

/* Parses header of HTTP request and adds them to a request's key:value map */
void RequestParser::parseHeaders()
{   
    std::string headers_section;
    if (!extractHeadersSection(headers_section))
        return;
    
    if (!processHeaderLines(headers_section))
        return;
    
    if (!validateRequiredHeaders())
        return;
    
    if (!parseBodyMetadata())
        return;
    
}

// ============================================================================
// BODY PARSING
// ============================================================================

/* Parses and validates Chunk size from the hex value that preceeds the body chunk */
bool RequestParser::parseChunkSize(const std::string& hex_value, size_t& out_size)
{
    if (hex_value.empty())
    {
        setErrorAndReturn("empty chunk size", "");
        return false;
    }
    for (unsigned char c : hex_value)
    {
        if (!std::isxdigit(c))
        {
            setErrorAndReturn("invalid hex character in chunk size", hex_value);
            return false;
        }
    }
    
    try
    {
        out_size = std::stoul(hex_value, nullptr, 16);
    }
    catch (const std::invalid_argument&)
    {
        setErrorAndReturn("malformed chunk size", hex_value);
        return false;
    }
    catch (const std::out_of_range&)
    {
        setErrorAndReturn("chunk size out of range", "");
        return false;
    }
    
    if (out_size > MAX_CHUNK_SIZE)
    {
        setErrorAndReturn("chunk size too large", "");
        return false;
    }
    
    return true;
}
/* Extracts and validates Chunk Data that proceeds chunk_size information*/
bool RequestParser::extractChunkData(const std::string& chunked_section, size_t& pos, size_t chunk_size)
{
    if (pos + chunk_size + CRLF_LENGTH > chunked_section.length())
    {
        setErrorAndReturn("went past chunked_sections length: chunk data incomplete", "");
        return false;
    }
    
    std::string chunk_data = chunked_section.substr(pos, chunk_size);
    m_request.appendBody(chunk_data);
    pos += chunk_size;
    
    if (chunked_section.compare(pos, CRLF_LENGTH, CRLF) != 0)
    {
        setErrorAndReturn("chunk missing trailing CRLF", "");
        return false;
    }
    pos += CRLF_LENGTH;
    
    if (m_request.getBody().size() > MAX_BODY_SIZE)
    {
        setErrorAndReturn("body too large", "");
        return false;
    }
    
    return true;
}

/* Validates and parses the body section of HTTP request if Transfer-Encoding method = chunked */
void RequestParser::parseChunkedBody()
{
    size_t last_chunk_pos = m_buffer.find("0" + CRLF);
    if (last_chunk_pos == std::string::npos)
        return;
    
    size_t final_crlf = m_buffer.find(EMPTY_LINE, last_chunk_pos);
    if (final_crlf == std::string::npos)
        return;
    
    std::string chunked_section = m_buffer.substr(0, final_crlf + EMPTY_LINE_LENGTH);
    m_buffer.erase(0, final_crlf + EMPTY_LINE_LENGTH);
    size_t pos = 0;
    while (pos < chunked_section.length())
    {
        size_t chunk_size_line_end = chunked_section.find(CRLF, pos);
        if (chunk_size_line_end == std::string::npos)
            return setErrorAndReturn("malformed chunk: missing CRLF after size", "");
        
        std::string hex_value = chunked_section.substr(pos, chunk_size_line_end - pos);
        size_t chunk_size = 0;
        
        if (!parseChunkSize(hex_value, chunk_size))
            return;
        if (chunk_size == 0)
            break;
        pos = chunk_size_line_end + CRLF_LENGTH;
        
        if (!extractChunkData(chunked_section, pos, chunk_size))
            return;
    }
    
    m_state = ParserState::COMPLETE;
}
/* Validates and parses the body section of HTTP request if Transfer-Encoding method = content-length */
void RequestParser::parseContentLengthBody()
{
    size_t content_length = m_request.getContentLen();
    if (content_length == 0)
    {
        m_state = ParserState::COMPLETE;
        return;
    }
    
    if (m_buffer.size() < content_length)
        return;
    
    if (content_length > MAX_BODY_SIZE)
        return setErrorAndReturn("body too large", "");
    
    std::string body_data = m_buffer.substr(0, content_length);
    m_buffer.erase(0, content_length);
    
    m_request.appendBody(body_data);
    m_state = ParserState::COMPLETE;
}

/* Checks transfer encoding method of http request*/
void RequestParser::parseBody()
{
    if (m_request.getChunked())
    {
        parseChunkedBody();
    } 
    else {
        parseContentLengthBody();
    }
}

// ============================================================================
// DATA ACCUMULATION & VALIDATION
// ============================================================================

/* Accumulates incoming data and validates buffer state
 * @param data: incoming data chunk
 * @return: false if error or need more data, true if ready to parse
 */
bool RequestParser::fetchData(const std::string& data)
{
    if (m_state == ParserState::ERROR || m_state == ParserState::COMPLETE)
        return false;
    m_buffer += data;
    if (m_state != ParserState::BODY && m_buffer.size() > MAX_TOTAL_HEADER_SIZE)
    {
        setErrorAndReturn("buffer exceeded max header size", "");
        return false;
    }

    // ADD TIMEOUT CHECK HERE PROBABLY (do we have a time function?)

    if (m_state == ParserState::REQUEST_LINE)
    {
        if (m_buffer.find(CRLF) == std::string::npos)
            return false;
    }
    else if (m_state == ParserState::HEADERS)
    {
        if (m_buffer.find(EMPTY_LINE) == std::string::npos)
            return false;
    }


    return true; 
}

// ============================================================================
// PARSING STATE MACHINE
// ============================================================================

/* Feeds data into the parser and processes based on current state
 * @param data: chunk of HTTP request data
 * @return: false if ERROR state reached, true otherwise
 * Use: call repeatedly with incoming socket data until parsing completes
 */
bool RequestParser::parseClientRequest(const std::string& data)
{
    if (!fetchData(data))
    {
        if (m_state == ParserState::ERROR)
            return false;
        else
            return true;
    }

    if (m_state == ParserState::REQUEST_LINE)
    {
        parseRequestLine();
        if (m_state == ParserState::ERROR)
            return false;
    }

    if (m_state == ParserState::HEADERS)
    {
        parseHeaders();
        if (m_state == ParserState::ERROR)
            return false;
    }

    if (m_state == ParserState::BODY)
    {
        parseBody();
        if (m_state == ParserState::ERROR)
            return false;
    }

    return true;
}

// ============================================================================
// DEBUG UTILITIES
// ============================================================================

/* Prints detailed parser state and request contents for debugging
 * @param label: optional label to identify the debug point
 */
void RequestParser::debugState(const char* label) const
{
    const size_t preview_length = 80;
    
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
            << " \nbuffer_prefix=\"" << m_buffer.substr(0, preview_length) << "\"" 
            << "\n------------------------------------\n";
}