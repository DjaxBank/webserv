#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <cstdint>

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
		std::string m_raw_uri;
		std::string m_normalized_path;
		std::string m_query;
		HttpVersion m_version;
		std::map<std::string, std::string> m_headers;
		std::vector<uint8_t> m_body;
		bool m_chunked;
		size_t m_content_len;

	public:
		Request();
		Request(const Request& other);
		Request& operator=(const Request& other);
		~Request();

		const HttpMethod& getMethod() const;
		const std::string& getRawUri() const;
		const std::string& getQuery() const;
		const std::string& getPath() const;
		const HttpVersion& getVersion() const;
		const std::map<std::string, std::string>& getHeaders() const;
		const std::vector<uint8_t>& getBody() const;
		bool getChunked() const;
		size_t getContentLen() const;

		void setMethod(const HttpMethod& method);
		void setRawUri(const std::string& raw_uri);
		void setQuery(const std::string& query);
		void setPath(const std::string& path);
		void setVersion(const HttpVersion& version);
		void setHeaders(const std::map<std::string, std::string>& headers);
		void setChunked(bool chunked);
		void setContentLen(size_t len);

		void addHeader(const std::string& key, const std::string& value);
		void appendBody(const std::string& chunk);
};

#endif