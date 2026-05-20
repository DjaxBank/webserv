#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include <string>
#include <optional>
#include <exception>
#include "Request.hpp"

enum class ParserState
{
	REQUEST_LINE,
	HEADERS,
	BODY,
	COMPLETE,
	ERROR,
};

enum class ParseError
{
    None,
    Incomplete,
    InvalidRequestLine,
    UnsupportedMethod,
    InvalidHttpVersion,
	InvalidUriSyntax,
	UriTooLong,
	MissingHostHeader,
    InvalidHeaderSyntax,
	HeaderSectionTooLarge,
    UnsupportedTransferEncoding,
    ConflictingLengthFraming,
	InvalidContentLength,
	BodyTooLarge,
    InvalidChunkedFraming,
    InternalParserFailure
};

enum class ReplyStatus
{
    OK = 200,
	Created = 201,
    MovedPermanently = 301,
    BadRequest = 400,
	Forbidden = 403,
	NotFound = 404,
	MethodNotAllowed = 405,
    RequestTimeout = 408,
    ContentTooLarge = 413,
    UriTooLong = 414,
    RequestHeaderFieldsTooLarge = 431,
    InternalServerError = 500,
    NotImplemented = 501,
	Unset = 0
};

namespace HTTP_CONSTANT {
	inline constexpr size_t CRLF_LENGTH = 2;
	inline constexpr char CRLF[] = "\r\n";
	inline constexpr size_t EMPTY_LINE_LENGTH = 4;
	inline constexpr char EMPTY_LINE[] = "\r\n\r\n";
	inline constexpr char SCHEME_SUFFIX[] = "://";
	inline constexpr char AUTHORITY_PREFIX[] = "//";
	inline constexpr size_t MAX_HEADER_SIZE = 32768;
	inline constexpr size_t MAX_TOTAL_HEADER_SIZE = 262144;
	inline constexpr size_t MAX_BODY_SIZE = 100 * 1024 * 1024;
	inline constexpr size_t MAX_CHUNK_SIZE = 8 * 1024 * 1024;
	inline constexpr size_t MAX_CHUNKED_BUFFER_SIZE = 16 * 1024 * 1024;
}

std::string method_tostring(HttpMethod method);
HttpMethod string_tomethod(const std::string& str);

void handle_method(HttpMethod method);


class HttpParseException : public std::exception
{
	private:
		ParseError m_error;
		ReplyStatus m_status;
		std::string m_msg;
	public:
		HttpParseException(const ParseError error, const ReplyStatus status, const std::string& msg);
		ParseError getError() const noexcept;
		ReplyStatus getStatus() const noexcept;
		const char* what() const noexcept override;
};


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
		std::optional<Request> parseClientRequest(const std::string& data);
		void debugState(const char* label = "DEBUG") const;

	private:
		std::string getHeader(const std::string& key) const;
		bool fetchData(const std::string& data);
		void parseRequestLine();
		void parseHeaders();
		void parseBody();
		void parseContentLengthBody();
		void parseChunkedBody();
		bool extractLineToken(std::string& source, std::string& out_token);
		void extractMethod(std::string& request_line);
		void extractTarget(std::string& request_line);
		void extractVersion(const std::string& version_token);
		bool extractHeadersSection(std::string& out_headers_section);
		void validateCRLF(const std::string &headers);
        void parseHeaderLine(const std::string& header_line);
        void processHeaderLines(const std::string& headers_section);
		void validateRequiredHeaders();
		void parseBodyMetadata();
		bool validateHTTPVersion(const std::string& version);
		void validateContentLength(const std::string& value, size_t& out_length);
		void parseChunkSize(const std::string& hex_value, size_t& out_size);
		std::string extractKey(const std::string& header_token);
        std::string extractValue(const std::string& header_token);
        std::string trimValue(const std::string& value);

		void parseURI(void);
		void pathTooLong(const std::string& working_uri);
		void validateLeadingSlash(const std::string& working_uri);
		void errorOnScheme(const std::string& working_uri);
		void errorOnAuthority(const std::string& working_uri);
		void errorOnEmpty(const std::string& working_uri);
		void errorOnUserInfo(const std::string& working_uri);
		void storeQuery(std::string& working_uri);
		void trimFragment(std::string& working_uri);

		void normalizeURI(std::string& parsed_uri);
		void rejectNullBytes(std::string& parsed_uri);
		void validateHexBytes(std::string& parsed_uri);
		char decodeByte(char c1, char c2);
		void normalizePath(std::string& parsed_uri);
};

#endif