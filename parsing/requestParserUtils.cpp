#include "requestParser.hpp"
#include <iostream>
#include <cctype>
#include <map>

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

/* Sets parser state to ERROR and logs debug info
 * @param reason: optional error description
 * @param line: the problematic line being parsed
 */
void RequestParser::setErrorAndReturn(const char* reason, const std::string& line)
{
    std::cerr << "[ParserError] state=" << static_cast<int>(m_state)
              << " reason=\"" << (reason ? reason : "") << "\""
              << " line=\"" << line << std::endl;
    m_state = ParserState::ERROR;
}

/* Wrapper for parsing failures */
bool RequestParser::fail(const char* reason, const std::string& line)
{
    setErrorAndReturn(reason, line);
    return false;
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

/* Extracts next complete line (up to \r\n) from source string
 * Removes the extracted line + CRLF from source
 * @param source: string to extract from (modified in place)
 * @param out_token: extracted line without CRLF
 * @return: true if line found, false if incomplete
 */
bool RequestParser::extractLineToken(std::string& source, std::string& out_token)
{
    size_t line_end = source.find(HTTP_CONSTANT::CRLF);
    if (line_end == std::string::npos)
        return false;
    
    out_token = source.substr(0, line_end);
    source.erase(0, line_end + HTTP_CONSTANT::CRLF_LENGTH);
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
    
    if (out_length > HTTP_CONSTANT::MAX_BODY_SIZE)
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
            << " \ntarget=\"" << m_request.getRawUri() << "\""
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
