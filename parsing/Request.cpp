#include "Request.hpp"
#include <algorithm>
#include <cctype>


/* Helper function to convert string to lowercase for case-insensitive header comparison
	@param str string you wanna lower, duh 
	*/
static std::string toLower(const std::string& str)
{
	std::string lower = str;
	for (size_t i = 0; i < lower.length(); ++i)
	{
		lower[i] = std::tolower(static_cast<unsigned char>(lower[i]));
	}
	return lower;
}

Request::Request() : m_method(), m_raw_uri(), m_version(), m_headers(), m_body(), m_chunked(), m_content_len()
{
};

Request::Request(const Request& other): m_method(other.m_method), m_raw_uri(other.m_raw_uri), m_version(other.m_version), m_headers(other.m_headers), m_body(other.m_body), m_chunked(other.m_chunked), m_content_len(other.m_content_len)
{
};

Request &Request::operator=(const Request& other)
{
	if (this != &other)
	{
		this->m_method = other.m_method;
		this->m_raw_uri = other.m_raw_uri;
		this->m_version = other.m_version;
		this->m_headers = other.m_headers;
		this->m_body = other.m_body;
		this->m_chunked = other.m_chunked;
		this->m_content_len = other.m_content_len;
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

const std::string& Request::getRawUri() const
{
	return(this->m_raw_uri);
}

const std::string& Request::getQuery() const
{
	return(this->m_query);
}

const std::string& Request::getPath() const
{
	return(this->m_normalized_path);
}

const HttpVersion& Request::getVersion() const
{
	return(this->m_version);
}

const std::map<std::string, std::string>& Request::getHeaders() const
{
	return(this->m_headers);
}

const std::vector<uint8_t>& Request::getBody() const
{
	return(this->m_body);
}

bool Request::getChunked() const
{
	return(this->m_chunked);
}
size_t Request::getContentLen() const
{
	return(this->m_content_len);
}

void Request::setMethod(const HttpMethod& method)
{
	m_method = method;
}
void Request::setRawUri(const std::string& raw_uri)
{
	m_raw_uri = raw_uri;
}

void Request::setQuery(const std::string& query)
{
	m_query = query;
}

void Request::setPath(const std::string& path)
{
	m_normalized_path = path;
}
void Request::setVersion(const HttpVersion& version)
{
	m_version = version;
}
void Request::setHeaders(const std::map<std::string, std::string>& headers)
{
	m_headers = headers;
}

void Request::setChunked(bool chunked)
{
	m_chunked = chunked;
}

void Request::setContentLen(size_t len)
{
	m_content_len = len;
}

void Request::addHeader(const std::string& key, const std::string& value)
{
	// TODO: handle duplicate headers (some headers like Set-Cookie can appear multiple times)
	// Current implementation only stores the last value for duplicate keys
	// so pretty much search the map and see if it exists already before adding a new one.
	m_headers[toLower(key)] = value;
}

void Request::appendBody(const std::string& chunk)
{
	m_body.insert(m_body.begin(), chunk.begin(), chunk.end());
}

