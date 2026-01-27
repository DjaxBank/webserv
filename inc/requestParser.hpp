#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include <string>
#include "../inc/Request.hpp"

enum class ParserState
{
	REQUEST_LINE,
	HEADERS,
	BODY,
	COMPLETE,
	ERROR,
};

std::string method_tostring(HttpMethod method);
HttpMethod string_tomethod(const std::string& str);

// this will be put in request handling later
// implementation example in requestParser.cpp
void handle_method(HttpMethod method);

class RequestParser
{
	private:
		std::string m_buffer;
		ParserState m_state;
		Request m_request;
		static const size_t MAX_HEADER_SIZE;
		static const size_t MAX_TOTAL_HEADER_SIZE;
		static const size_t MAX_BODY_SIZE;
		static const size_t MAX_CHUNK_SIZE;

		bool m_need_chunk_size = true;
		size_t m_chunkBytesRemaining = 0;
		size_t m_chunkBytesReceived = 0;
		bool   m_parsingTrailers = false;

	public:
		RequestParser();
		RequestParser(const RequestParser& other);
		RequestParser& operator=(const RequestParser& other);
		~RequestParser();
		ParserState getState() const;
		bool fetch_data(const std::string& data);
		void debugState(const char* label = "DEBUG") const;
	
		const HttpMethod& getMethod() const;
		const std::string& getTarget() const;
		const HttpVersion& getVersion() const;
		const std::string getHeader(const std::string& key) const;
		const std::vector<uint8_t>& getBody() const;
	private:
		void parseRequestLine();
		void parseHeaders();
		void parseBody();
		void setErrorAndReturn(const char* reason = "", const std::string& line = "");
		bool fail(const char* reason = "", const std::string& line = "");
		bool validateRequiredHeaders();
		bool parseBodyMetadata();
		bool validateHTTPVersion(const std::string& version);
		bool validateContentLength(const std::string& value, size_t& out_length);
		std::string extractKey(const std::string& header_token);
        std::string extractValue(const std::string& header_token);
        std::string trimValue(const std::string& value);
};

#endif