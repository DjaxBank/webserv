#include <../inc/Request.hpp>

Request::Request() : m_method(), m_path(), m_version(), m_headers(), m_body()
{
};

Request::Request(const Request& other): m_method(other.m_method), m_path(other.m_path), m_version(other.m_version), m_headers(other.m_headers), m_body(other.m_body)
{
};

Request &Request::operator=(const Request& other)
{
	if (this != &other)
	{
		this->m_method = other.m_method;
		this->m_path = other.m_path;
		this->m_version = other.m_version;
		this->m_headers = other.m_headers;
		this->m_body = other.m_body;
	}
	return *this;
}

Request::~Request()
{
};

const std::string& Request::getMethod() const
{
	return(this->m_method);
}

const std::string& Request::getPath() const
{
	return(this->m_path);
}

const std::string& Request::getVersion() const
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

void Request::setMethod(const std::string& method)
{
	m_method = method;
}
void Request::setPath(const std::string& path)
{
	m_path = path;
}
void Request::setVersion(const std::string& version)
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