#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

enum class HttpMethod
{
	NONE,
	GET,
	POST,
	DELETE,
};

enum class HttpVersion
{
	NONE,
	HTTP_1_0,
	HTTP_1_1,
};

class Request
{
	private:
		HttpMethod m_method;
		std::string m_target;
		HttpVersion m_version;
		std::map<std::string, std::string> m_headers;
		std::string m_body;
		bool m_chunked;
		size_t m_content_len;
	public:
		Request();
		Request(const Request& other);
		Request& operator=(const Request& other);
		~Request();

		const HttpMethod& getMethod() const;
		const std::string& getTarget() const;
		const HttpVersion& getVersion() const;
		const std::map<std::string, std::string>& getHeaders() const;
		const std::string& getBody() const;
		bool getChunked() const;
		size_t getContentLen() const;

		void setMethod(const HttpMethod& method);
		void setTarget(const std::string& target);
		void setVersion(const HttpVersion& version);
		void setHeaders(const std::map<std::string, std::string>& headers);
		void setBody(const std::string& body);
		void setChunked(bool chunked);
		void setContentLen(size_t len);

		void addHeader(const std::string& key, const std::string& value);
};

#endif