#include "../inc/requestParser.hpp"
#include "../inc/Request.hpp"
#include <iostream>
#include <map>
#include <cctype>

// Forward declarations for functions defined in requestParserUtils.cpp
std::string method_tostring(HttpMethod method);
std::string version_tostring(const HttpVersion &version);
std::string state_tostring(ParserState state);

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
    // Convert key to lowercase for case-insensitive lookup
    std::string lowerKey = key;
    for (size_t i = 0; i < lowerKey.length(); ++i)
        lowerKey[i] = std::tolower(static_cast<unsigned char>(lowerKey[i]));
    
    auto iterator = headers.find(lowerKey);
    if (iterator != headers.end())
    {
        return iterator->second;
    }
    else
        return "";
}

/* Accumulates incoming data and validates buffer state
 * @param data: incoming data chunk
 * @return: false if error or need more data, true if ready to parse
 * Moves onto parsing when that section's data is ready to process
 */
bool RequestParser::fetchData(const std::string& data)
{
    if (m_state == ParserState::ERROR || m_state == ParserState::COMPLETE)
        return false;
    m_buffer += data;
    if (m_state != ParserState::BODY && m_buffer.size() > HTTP_CONSTANT::MAX_TOTAL_HEADER_SIZE)
    {
        setErrorAndReturn("buffer exceeded max header size", "");
        return false;
    }

    // ADD TIMEOUT CHECK HERE PROBABLY (do we have a time function?)

    if (m_state == ParserState::REQUEST_LINE)
    {
        if (m_buffer.find(HTTP_CONSTANT::CRLF) == std::string::npos)
            return false;
    }
    else if (m_state == ParserState::HEADERS)
    {
        if (m_buffer.find(HTTP_CONSTANT::EMPTY_LINE) == std::string::npos)
            return false;
    }


    return true;
}

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