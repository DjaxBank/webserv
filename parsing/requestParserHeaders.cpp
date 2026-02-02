#include "../inc/requestParser.hpp"
#include <iostream>

/* Extracts the complete headers section from buffer
 * @param out_headers_section: extracted headers without trailing \r\n\r\n
 * @return: true if complete section found, false if incomplete
 */
bool RequestParser::extractHeadersSection(std::string& out_headers_section)
{
    size_t header_end = m_buffer.find(HTTP_CONSTANT::EMPTY_LINE);
    if (header_end == std::string::npos)
        return false;
    
    if (header_end > HTTP_CONSTANT::MAX_TOTAL_HEADER_SIZE)
    {
        setErrorAndReturn("total header size too large", "");
        return false;
    }
    
    out_headers_section = m_buffer.substr(0, header_end);
    m_buffer.erase(0, header_end + HTTP_CONSTANT::EMPTY_LINE_LENGTH);
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
    
    if (header_line.length() > HTTP_CONSTANT::MAX_HEADER_SIZE)
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
        size_t line_end = headers_section.find(HTTP_CONSTANT::CRLF, pos);
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
        
        pos = line_end + HTTP_CONSTANT::CRLF_LENGTH;
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
        // TODO: consider rejecting oversized bodies here instead of waiting until parseContentLengthBody
        // would save memory by not buffering data we'll reject anyway
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
