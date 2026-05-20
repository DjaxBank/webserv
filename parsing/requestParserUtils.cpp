#include "requestParser.hpp"
#include <iostream>
#include <cctype>
#include <map>

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

HttpMethod string_tomethod(const std::string& str)
{
    if (str == "GET") return HttpMethod::GET;
    if (str == "POST") return HttpMethod::POST;
    if (str == "DELETE") return HttpMethod::DELETE;
    return HttpMethod::NONE;
}

HttpVersion string_toversion(const std::string &version)
{
    if (version == "HTTP/1.0") return HttpVersion::HTTP_1_0;
    if (version == "HTTP/1.1") return HttpVersion::HTTP_1_1;
    return HttpVersion::NONE;
}

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

void handle_method(HttpMethod method)
{
    switch (method)
    {
        case HttpMethod::GET:
            break; 
        case HttpMethod::POST:
            break; 
        case HttpMethod::DELETE:
            break; 
        case HttpMethod::NONE:
           break;
    }
}

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

bool RequestParser::extractLineToken(std::string& source, std::string& out_token)
{
    size_t line_end = source.find(HTTP_CONSTANT::CRLF);
    if (line_end == std::string::npos)
        return false;
    
    out_token = source.substr(0, line_end);
    source.erase(0, line_end + HTTP_CONSTANT::CRLF_LENGTH);
    return true;
}

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

std::string RequestParser::extractValue(const std::string& header_token)
{
    size_t pos = header_token.find(':');
    if (pos == std::string::npos)
        return "";
   std::string value = header_token.substr(pos + 1);
    return trimValue(value);
}


bool RequestParser::validateHTTPVersion(const std::string& version)
{
    return version == "HTTP/1.0"
        || version == "HTTP/1.1";
}

void RequestParser::validateContentLength(const std::string& value, size_t& out_length)
{
    if (value.empty())
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
                ParseError::InvalidContentLength, 
                ReplyStatus::BadRequest, 
                "Invalid content length."
            );
    }

    for (unsigned char c : value)
    {
        if (!std::isdigit(c))
        {
            m_state = ParserState::ERROR;
            throw HttpParseException(
                ParseError::InvalidContentLength, 
                ReplyStatus::BadRequest, 
                "Invalid content length."
            );
        }
    }

    try
    {
        out_length = std::stoul(value, nullptr, 10);
    }
    catch (const std::invalid_argument&)
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
                ParseError::InvalidContentLength, 
                ReplyStatus::BadRequest, 
                "Invalid content length."
            );
    }
    catch (const std::out_of_range&)
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
                ParseError::BodyTooLarge, 
                ReplyStatus::ContentTooLarge, 
                "Body too large."
            );
    }
    
    if (out_length > HTTP_CONSTANT::MAX_BODY_SIZE)
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
                ParseError::BodyTooLarge, 
                ReplyStatus::ContentTooLarge, 
                "Body too large."
            );
    }
}

void RequestParser::validateRequiredHeaders()
{
    if (m_request.getVersion() == HttpVersion::HTTP_1_1 && getHeader("Host").empty())
    {
        m_state = ParserState::ERROR;
        throw HttpParseException(
            ParseError::MissingHostHeader,
            ReplyStatus::BadRequest,
            "Missing host header."
        );
    }
}

void RequestParser::debugState(const char* label) const
{
    const size_t preview_length = 80;
    
    std::cerr << "\n------------------------------------"
            << "\n[ParserState] " << (label ? label : "")
            << " \nstate=" << state_tostring(m_state) << "\""
            << " \nmethod=\"" << method_tostring(m_request.getMethod()) << "\""
            << " \nraw_uri=\"" << m_request.getRawUri() << "\""
            << " \nnormalized_path\"" << m_request.getPath() << "\""
            << " \nversion=\"" << version_tostring(m_request.getVersion()) << "\""
            << " \nheaders_count=\"" << m_request.getHeaders().size() << "\""
             << " \nheaders=\"" << m_request.printHeaders() << "\""
            << " \nbody=" << m_request.getBodyAsString() << "\n";
            
    const std::map<std::string, std::string>& headers = m_request.getHeaders();
    for (const auto& pair : headers)
    {
        std::cerr << "  [" << pair.first << "]: " << pair.second << "\n";
    }
    
    std::cerr << " \nbody_bytes=" << m_request.getBody().size()
            << " \nbuffer_prefix=\"" << m_buffer.substr(0, preview_length) << "\"" 
            << "\n------------------------------------\n";
}
