#include "requestParser.hpp"
#include <iostream>
#include <cctype>

/* Strips chunk extensions from chunk size line
 * HTTP allows "5;name=value" but we ignore extensions
 * @param chunk_line: the full chunk size line (may contain extensions)
 * @return: just the hex portion before any ';'
 */
static std::string stripChunkExtensions(const std::string& chunk_line)
{
    size_t semicolon = chunk_line.find(';');
    if (semicolon != std::string::npos)
        return chunk_line.substr(0, semicolon);
    return chunk_line;
}

/* Parses and validates Chunk size from the hex value that preceeds the body chunk */
bool RequestParser::parseChunkSize(const std::string& hex_value, size_t& out_size)
{
    if (hex_value.empty())
    {
        HttpParseException(
                ParseError::InvalidChunkedFraming, 
                ReplyStatus::BadRequest, 
                "Malformed Chunk: Empty chunk size."
            );
    }
    for (unsigned char c : hex_value)
    {
        if (!std::isxdigit(c))
        {
            HttpParseException(
                ParseError::InvalidChunkedFraming, 
                ReplyStatus::BadRequest, 
                "Malformed Chunk: Invalid hex character in chunk size"
            );
        }
    }
    
    try
    {
        out_size = std::stoul(hex_value, nullptr, 16);
    }
    catch (const std::invalid_argument&)
    {
        HttpParseException(
                ParseError::InvalidChunkedFraming, 
                ReplyStatus::BadRequest, 
                "Malformed Chunk: Malformed chunk size."
            );
    }
    catch (const std::out_of_range&)
    {
        HttpParseException(
                ParseError::InvalidChunkedFraming, 
                ReplyStatus::BadRequest, 
                "Malformed Chunk: Chunk size out of range."
            );
    }
    
    if (out_size > HTTP_CONSTANT::MAX_CHUNK_SIZE)
    {
        HttpParseException(
                ParseError::InvalidChunkedFraming, 
                ReplyStatus::BadRequest, 
                "Malformed Chunk: Chunk size too large."
            );
    }
    
    return true;
}

/* Extracts and validates Chunk Data that proceeds chunk_size information*/
bool RequestParser::extractChunkData(const std::string& chunked_section, size_t& pos, size_t chunk_size)
{
    if (pos + chunk_size + HTTP_CONSTANT::CRLF_LENGTH > chunked_section.length())
    {
        HttpParseException(
                ParseError::InvalidChunkedFraming, 
                ReplyStatus::BadRequest, 
                "Got past chunked_sections length: chunk data incomplete."
            );
    }
    
    std::string chunk_data = chunked_section.substr(pos, chunk_size);
    m_request.appendBody(chunk_data);
    pos += chunk_size;
    
    if (chunked_section.compare(pos, HTTP_CONSTANT::CRLF_LENGTH, HTTP_CONSTANT::CRLF) != 0)
    {
        HttpParseException(
                ParseError::InvalidChunkedFraming, 
                ReplyStatus::BadRequest, 
                "Malformed Chunk: missing CRLF after size."
            );
    }
    pos += HTTP_CONSTANT::CRLF_LENGTH;
    
    if (m_request.getBody().size() > HTTP_CONSTANT::MAX_BODY_SIZE)
    {
        HttpParseException(
                ParseError::BodyTooLarge, 
                ReplyStatus::ContentTooLarge, 
                "Body too large."
            );
    }
    
    return true;
}

/* Validates and parses the body section of HTTP request if Transfer-Encoding method = chunked */
void RequestParser::parseChunkedBody()
{
    size_t last_chunk_pos = m_buffer.find(std::string("0") + HTTP_CONSTANT::CRLF);
    if (last_chunk_pos == std::string::npos)
        return;
    
    size_t final_crlf = m_buffer.find(HTTP_CONSTANT::EMPTY_LINE, last_chunk_pos);
    if (final_crlf == std::string::npos)
        return;
    
    std::string chunked_section = m_buffer.substr(0, final_crlf + HTTP_CONSTANT::EMPTY_LINE_LENGTH);
    m_buffer.erase(0, final_crlf + HTTP_CONSTANT::EMPTY_LINE_LENGTH);
    size_t pos = 0;
    while (pos < chunked_section.length())
    {
        size_t chunk_size_line_end = chunked_section.find(HTTP_CONSTANT::CRLF, pos);
        if (chunk_size_line_end == std::string::npos)
        {
            HttpParseException(
                ParseError::InvalidChunkedFraming, 
                ReplyStatus::BadRequest, 
                "Malformed Chunk: missing CRLF after size."
            );
        }
        std::string chunk_line = chunked_section.substr(pos, chunk_size_line_end - pos);
        std::string hex_value = stripChunkExtensions(chunk_line);
        size_t chunk_size = 0;
        
        if (!parseChunkSize(hex_value, chunk_size))
            return;
        if (chunk_size == 0)
            break;
        pos = chunk_size_line_end + HTTP_CONSTANT::CRLF_LENGTH;
        
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
    
    if (content_length > HTTP_CONSTANT::MAX_BODY_SIZE)
    {
        HttpParseException(
                ParseError::BodyTooLarge, 
                ReplyStatus::ContentTooLarge, 
                "Body too large."
            );
    }
    
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
