#include "requestParser.hpp"
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
        throw HttpParseException(
            ParseError::HeaderSectionTooLarge, 
            ReplyStatus::RequestHeaderFieldsTooLarge, 
            "Header fields too large."
        );
    }
    
    out_headers_section = m_buffer.substr(0, header_end);
    m_buffer.erase(0, header_end + HTTP_CONSTANT::EMPTY_LINE_LENGTH);
    return true;
}
 
#include <array>
#include <string_view>
#include <cstdint>

constexpr static std::array<bool, 256> valid_key_chars = []()
{
	std::array<bool, 256> result{};
	const std::string_view safe_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "!#$%&'*+-.^_"
    "\x60"
    "|~";
	for (uint8_t i : safe_chars)
	{
		result[i] = true; 
	}
	return result;
}();

constexpr static std::array<bool, 256> valid_value_chars = []()
{
	std::array<bool, 256> result{};
	const std::string_view safe_chars =
        " "
        "!\"#$%&'()*+,-./"
        "0123456789"
        ":;<=>?@"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "[\\]^_`"
        "abcdefghijklmnopqrstuvwxyz"
        "{|}~";
	for (uint8_t i : safe_chars)
	{
		result[i] = true; 
	}
	return result;
}();


static bool check_safe_key_char(char c)
{
	return valid_key_chars[static_cast<unsigned char>(c)];
}

static bool check_safe_value_char(char c)
{
	return valid_value_chars[static_cast<unsigned char>(c)];
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
        throw HttpParseException(
            ParseError::HeaderSectionTooLarge, 
            ReplyStatus::RequestHeaderFieldsTooLarge, 
            "Header line too large."
        );
    }
    
    std::string key = extractKey(header_line);
    for (unsigned char c : key)
    {
        if (!check_safe_key_char(c))
        {
            throw HttpParseException(
                ParseError::InvalidHeaderSyntax, 
                ReplyStatus::BadRequest, 
                "Header contains UNSAFE character."
            );
        }
    }
    if (key.empty())
    {
        throw HttpParseException(
            ParseError::InvalidHeaderSyntax, 
            ReplyStatus::BadRequest, 
            "Header contains UNSAFE character."
        );
    }
    
    std::string value = extractValue(header_line);
    for (unsigned char c : value)
    {
        if (!check_safe_value_char(c))
        {
            throw HttpParseException(
                ParseError::InvalidHeaderSyntax, 
                ReplyStatus::BadRequest, 
                "Header contains UNSAFE character."
            );
        }
    }
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
    {
        throw HttpParseException(
                ParseError::ConflictingLengthFraming, 
                ReplyStatus::BadRequest, 
                "Transfer-Encoding and Content-Length are both present."
            );
    }

    if (!t_encoding.empty())
    {
        std::string encoding = trimValue(t_encoding);

        if (encoding == "chunked")
        {
            m_request.setChunked(true);
            m_state = ParserState::BODY;
            return true;
        }
        throw HttpParseException(
                ParseError::UnsupportedTransferEncoding, 
                ReplyStatus::BadRequest, 
                "Unsupported Transfer Encoding method."
            );
    }

    if (!c_length.empty())
    {
        size_t content_len;

        validateContentLength(c_length, content_len);
        m_request.setContentLen(content_len);
        m_state = ParserState::BODY;
        return true;
    }
    
    m_state = ParserState::COMPLETE;
    return true;
}

bool RequestParser::validateCRLF(const std::string &string)
{
    for (size_t i = 0; i < string.size(); i++)
    {
        if (string[i] == '\n')
        {
            if (i == 0 || string[i - 1] != '\r')
            {
                if (m_state == ParserState::REQUEST_LINE)
                {
                    throw HttpParseException(
                        ParseError::InvalidRequestLine, 
                        ReplyStatus::BadRequest, 
                        "Found bare '\n' in request line."
                    );
                }
                else
                {
                    throw HttpParseException(
                        ParseError::InvalidHeaderSyntax, 
                        ReplyStatus::BadRequest, 
                        "Found bare '\n' in header line."
                    );
                }
            }
        }
        if (string[i] == '\r')
        {
            if (i + 1 >= string.size() || string[i + 1] != '\n')
            {
                if (m_state == ParserState::REQUEST_LINE)
                {
                    throw HttpParseException(
                        ParseError::InvalidRequestLine, 
                        ReplyStatus::BadRequest, 
                        "Found bare '\r' in request line."
                    );
                }
                else
                {
                    throw HttpParseException(
                        ParseError::InvalidHeaderSyntax, 
                        ReplyStatus::BadRequest, 
                        "Found bare '\r' in header line."
                    );
                }
            }    
        }
    }
    return true;
}

/* Parses header of HTTP request and adds them to a request's key:value map */
void RequestParser::parseHeaders()
{   
    std::string headers_section;
    if (!extractHeadersSection(headers_section))
        return;

    if (!validateCRLF(headers_section))
        return;
    
    if (!processHeaderLines(headers_section))
        return;
    
    if (!validateRequiredHeaders())
        return;
    
    if (!parseBodyMetadata())
        return;
    
}
