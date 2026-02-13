#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include <string>
#include "Request.hpp"

enum class ParserState
{
	REQUEST_LINE,
	HEADERS,
	BODY,
	COMPLETE,
	ERROR,
};

// HTTP parsing constants
namespace HTTP_CONSTANT {
	inline constexpr size_t CRLF_LENGTH = 2;
	inline constexpr char CRLF[] = "\r\n";
	inline constexpr size_t EMPTY_LINE_LENGTH = 4;
	inline constexpr char EMPTY_LINE[] = "\r\n\r\n";
	inline constexpr char SCHEME_SUFFIX[] = "://";
	inline constexpr char AUTHORITY_PREFIX[] = "//";
	// Max size of a single header line must be less than 32kb
	inline constexpr size_t MAX_HEADER_SIZE = 32768;
	// Max size of headers must be less than 256kb
	inline constexpr size_t MAX_TOTAL_HEADER_SIZE = 262144;
	// Max size of body must be 100MB or less
	inline constexpr size_t MAX_BODY_SIZE = 100 * 1024 * 1024;
	// Max size of chunks must be 8MB or less
	inline constexpr size_t MAX_CHUNK_SIZE = 8 * 1024 * 1024;
}

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

	public:
		RequestParser();
		RequestParser(const RequestParser& other);
		RequestParser& operator=(const RequestParser& other);
		~RequestParser();
		ParserState getState() const;
		bool parseClientRequest(const std::string& data);
		void debugState(const char* label = "DEBUG") const;
	
		const HttpMethod& getMethod() const;
		const std::string& getTarget() const;
		const HttpVersion& getVersion() const;
		const std::string getHeader(const std::string& key) const;
		const std::vector<uint8_t>& getBody() const;
	private:
		bool fetchData(const std::string& data);
		void parseRequestLine();
		void parseHeaders();
		void parseBody();
		void parseContentLengthBody();
		void parseChunkedBody();
		void setErrorAndReturn(const char* reason = "", const std::string& line = "");
		bool extractLineToken(std::string& source, std::string& out_token);
		bool extractMethod(std::string& request_line);
		bool extractTarget(std::string& request_line);
		bool extractVersion(const std::string& version_token);
		bool extractHeadersSection(std::string& out_headers_section);
        bool parseHeaderLine(const std::string& header_line);
        bool processHeaderLines(const std::string& headers_section);
		bool validateRequiredHeaders();
		bool fail(const char* reason = "", const std::string& line = "");
		bool parseBodyMetadata();
		bool validateHTTPVersion(const std::string& version);
		bool validateContentLength(const std::string& value, size_t& out_length);
		bool parseChunkSize(const std::string& hex_value, size_t& out_size);
		bool extractChunkData(const std::string& chunked_section, size_t& pos, size_t chunk_size);
		std::string extractKey(const std::string& header_token);
        std::string extractValue(const std::string& header_token);
        std::string trimValue(const std::string& value);

		// implementing URI parsing below
		bool parseURI(void);
		bool pathTooLong(const std::string& working_uri);
		bool validateLeadingSlash(const std::string& working_uri);
		bool errorOnScheme(const std::string& working_uri);
		bool errorOnAuthority(const std::string& working_uri);
		bool errorOnEmpty(const std::string& working_uri);
		bool errorOnUserInfo(const std::string& working_uri);
		bool storeQuery(std::string& working_uri);
		void trimFragment(std::string& working_uri);

		// normalize funcs
		bool normalizeURI(std::string& parsed_uri);
		bool rejectNullBytes(std::string& parsed_uri);
		bool validateHexBytes(std::string& parsed_uri);
		char decodeByte(char c1, char c2);
};

#endif