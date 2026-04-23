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

static bool extractChunkLine(const std::string& buffer, size_t start, std::string& out_line, size_t& out_next_pos)
{
    size_t line_end = buffer.find(HTTP_CONSTANT::CRLF, start);
    if (line_end == std::string::npos)
        return false;
    out_line = buffer.substr(start, line_end - start);
    out_next_pos = line_end + HTTP_CONSTANT::CRLF_LENGTH;
    return true;
}

static bool chunkPayloadIncomplete(const std::string& buffer, size_t payload_start, size_t chunk_size)
{
    return payload_start + chunk_size + HTTP_CONSTANT::CRLF_LENGTH > buffer.size();
}

/* Parses and validates Chunk size from the hex value that preceeds the body chunk */
void RequestParser::parseChunkSize(const std::string& hex_value, size_t& out_size)
{
    if (hex_value.empty())
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
                ParseError::InvalidChunkedFraming, 
                ReplyStatus::BadRequest,
                "Malformed Chunk: Empty chunk size."
            );
    }
    for (unsigned char c : hex_value)
    {
        if (!std::isxdigit(c))
        {
            m_state = ParserState::ERROR;
            throw HttpParseException(
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
        m_state = ParserState::ERROR;
        throw HttpParseException(
                ParseError::InvalidChunkedFraming, 
                ReplyStatus::BadRequest, 
                "Malformed Chunk: Malformed chunk size."
            );
    }
    catch (const std::out_of_range&)
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
                ParseError::InvalidChunkedFraming, 
                ReplyStatus::BadRequest, 
                "Malformed Chunk: Chunk size out of range."
            );
    }
    
    if (out_size > HTTP_CONSTANT::MAX_CHUNK_SIZE)
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
                ParseError::InvalidChunkedFraming, 
                ReplyStatus::BadRequest, 
                "Malformed Chunk: Chunk size too large."
            );
    }
    
}

/* Validates and parses the body section of HTTP request if Transfer-Encoding method = chunked */
void RequestParser::parseChunkedBody()
{
    size_t pos = 0;
    std::string parsed_body;

    while (true)
    {
        std::string chunk_line;
        size_t next_pos = 0;
        if (!extractChunkLine(m_buffer, pos, chunk_line, next_pos))
            return;
        std::string hex_value = stripChunkExtensions(chunk_line);
        size_t chunk_size = 0;
        parseChunkSize(hex_value, chunk_size);
        pos = next_pos;

        if (chunk_size == 0)
        {
            if (m_buffer.size() < pos + HTTP_CONSTANT::CRLF_LENGTH)
                return;
            if (m_buffer.compare(pos, HTTP_CONSTANT::CRLF_LENGTH, HTTP_CONSTANT::CRLF) != 0)
            {
                m_state = ParserState::ERROR;
                throw HttpParseException(
                        ParseError::InvalidChunkedFraming, 
                        ReplyStatus::BadRequest, 
                        "Malformed Chunk: invalid terminating chunk framing."
                    );
            }
            pos += HTTP_CONSTANT::CRLF_LENGTH;
            if (m_buffer.size() != pos)
            {
                m_state = ParserState::ERROR;
                throw HttpParseException(
                        ParseError::InvalidChunkedFraming, 
                        ReplyStatus::BadRequest, 
                        "Malformed Chunk: trailing bytes after chunked body."
                    );
            }
            m_buffer.erase(0, pos);
            m_request.appendBody(parsed_body);
            m_state = ParserState::COMPLETE;
            return;
        }

        if (chunkPayloadIncomplete(m_buffer, pos, chunk_size))
            return;

        parsed_body.append(m_buffer, pos, chunk_size);
        pos += chunk_size;
        if (m_buffer.compare(pos, HTTP_CONSTANT::CRLF_LENGTH, HTTP_CONSTANT::CRLF) != 0)
        {
            m_state = ParserState::ERROR;
            throw HttpParseException(
                    ParseError::InvalidChunkedFraming, 
                    ReplyStatus::BadRequest, 
                    "Malformed Chunk: missing CRLF after size."
                );
        }
        pos += HTTP_CONSTANT::CRLF_LENGTH;
        if (m_request.getBody().size() + parsed_body.size() > HTTP_CONSTANT::MAX_BODY_SIZE)
        {
            m_state = ParserState::ERROR;
            throw HttpParseException(
                    ParseError::BodyTooLarge, 
                    ReplyStatus::ContentTooLarge, 
                    "Body too large."
                );
        }
    }
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
        m_state = ParserState::ERROR;
        throw HttpParseException(
                ParseError::BodyTooLarge, 
                ReplyStatus::ContentTooLarge, 
                "Body too large."
            );
    }
    
    std::string body_data = m_buffer.substr(0, content_length);
    m_buffer.erase(0, content_length);
    if (!m_buffer.empty())
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
                ParseError::InvalidContentLength, 
                ReplyStatus::BadRequest, 
                "Invalid body framing: trailing bytes after Content-Length body."
            );
    }
    
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
