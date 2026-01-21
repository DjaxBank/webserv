#include "../inc/Request.hpp"

Request::Request() : m_method(), m_target(), m_version(), m_headers(), m_body()
{
};

Request::Request(const Request& other): m_method(other.m_method), m_target(other.m_target), m_version(other.m_version), m_headers(other.m_headers), m_body(other.m_body)
{
};

Request &Request::operator=(const Request& other)
{
	if (this != &other)
	{
		this->m_method = other.m_method;
		this->m_target = other.m_target;
		this->m_version = other.m_version;
		this->m_headers = other.m_headers;
		this->m_body = other.m_body;
	}
	return *this;
}

Request::~Request()
{
};

const HttpMethod& Request::getMethod() const
{
	return(this->m_method);
}

const std::string& Request::getTarget() const
{
	return(this->m_target);
}

const HttpVersion& Request::getVersion() const
{
	return(this->m_version);
}

const std::map<std::string, std::string>& Request::getHeaders() const
{
	return(this->m_headers);
}

const std::string& Request::getBody() const
{
	return(this->m_body);
}

void Request::setMethod(const HttpMethod& method)
{
	m_method = method;
}
void Request::setTarget(const std::string& path)
{
	m_target = path;
}
void Request::setVersion(const HttpVersion& version)
{
	m_version = version;
}
void Request::setHeaders(const std::map<std::string, std::string>& headers)
{
	m_headers = headers;
}
void Request::setBody(const std::string& body)
{
    m_body = body;
}

void Request::addHeader(const std::string& key, const std::string& value)
{
	m_headers[key] = value;
}