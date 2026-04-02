#include "requestParser.hpp"

HttpParseException::HttpParseException(int status, const std::string& msg) : m_status(status), m_msg(msg) {}

ParseError HttpParseException::getError() const noexcept
{
	return m_error;
}

ReplyStatus HttpParseException::getStatus() const noexcept
{
	return m_status;
}

const char* HttpParseException::what() const noexcept
{
	return m_msg.c_str();
}