#include "requestParser.hpp"
#include <iostream>

HttpMethod string_tomethod(const std::string& str);
HttpVersion string_toversion(const std::string& version);

/* Extracts and validates HTTP method from request line
 * Modifies request_line by removing method and following space
 * @return: true if valid method found, false otherwise
 */
void RequestParser::extractMethod(std::string& request_line)
{
    size_t pos = request_line.find(' ');
    if (pos == std::string::npos)
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
            ParseError::InvalidRequestLine, 
            ReplyStatus::BadRequest,
            "No space after method"    
        );
    }
    
    std::string method_token = request_line.substr(0, pos);
    HttpMethod method = string_tomethod(method_token);
    if (method == HttpMethod::NONE)
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
            ParseError::UnsupportedMethod, 
            ReplyStatus::BadRequest,
            "Unsupported Method."    
        );
    }
    
    m_request.setMethod(method);
    request_line = request_line.substr(pos + 1);
}

/* Extracts and validates target URI from request line
 * Modifies request_line by removing target and following space
 * @return: true if valid target found, false otherwise
 */
void RequestParser::extractTarget(std::string& request_line)
{
    size_t pos = request_line.find(' ');
    if (pos == std::string::npos)
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
            ParseError::InvalidRequestLine, 
            ReplyStatus::BadRequest,
            "No space after target"    
        );
    }
    
    std::string target_token = request_line.substr(0, pos);
    if (target_token.empty())
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
            ParseError::InvalidRequestLine, 
            ReplyStatus::BadRequest,
            "Empty target."    
        );
    }
    m_request.setRawUri(target_token);
    request_line = request_line.substr(pos + 1);
}

/* Extracts and validates HTTP version from request line
 * @param version_token: the version string (should be last token)
 * @return: true if valid version, false otherwise
 */
void RequestParser::extractVersion(const std::string& version_token)
{
    if (version_token.empty() || version_token.find(' ') != std::string::npos)
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
            ParseError::InvalidHttpVersion, 
            ReplyStatus::BadRequest,
            "Version missing or spaces present in version."    
        );
    }
    if (!validateHTTPVersion(version_token))
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
            ParseError::InvalidHttpVersion, 
            ReplyStatus::BadRequest,
            "Invalid HTTP version."    
        );
    }
    m_request.setVersion(string_toversion(version_token));
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
    validateCRLF(request_line);
    extractMethod(request_line);
    
    extractTarget(request_line);

    parseURI();
    extractVersion(request_line);
    m_state = ParserState::HEADERS;
}
