#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <optional>
#include "requestParser.hpp"

enum class Expectation {
    SUCCESS,
    ERROR,
    INCOMPLETE,
};

struct ExpectedRequest {
    std::optional<HttpMethod> method;
    std::optional<std::string> path;
    std::optional<std::string> query;
    std::optional<HttpVersion> version;
    std::optional<bool> chunked;
    std::optional<size_t> contentLength;
    std::optional<std::string> body;
};

struct TestCase {
    std::string name;
    std::vector<std::string> chunks;
    Expectation expectation;
    ExpectedRequest expected;
    bool debugPrintHeaders;
};

static std::string toText(HttpMethod method)
{
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::NONE: return "NONE";
    }
    return "NONE";
}

static std::string toText(HttpVersion version)
{
    switch (version) {
        case HttpVersion::HTTP_1_0: return "HTTP/1.0";
        case HttpVersion::HTTP_1_1: return "HTTP/1.1";
        case HttpVersion::NONE: return "NONE";
    }
    return "NONE";
}

static std::string toText(Expectation expectation)
{
    switch (expectation) {
        case Expectation::SUCCESS: return "success";
        case Expectation::ERROR: return "error";
        case Expectation::INCOMPLETE: return "incomplete";
    }
    return "unknown";
}

static std::string makeLongHeaderRequest(size_t valueLen)
{
    return "GET / HTTP/1.1\r\nHost: example.com\r\nX-Long: "
        + std::string(valueLen, 'a') + "\r\n\r\n";
}

static std::string makeHugeHeaderBlockRequest(size_t lines, size_t valueLen)
{
    std::string req = "GET / HTTP/1.1\r\nHost: example.com\r\n";
    for (size_t i = 0; i < lines; ++i) {
        req += "X-" + std::to_string(i) + ": " + std::string(valueLen, 'b') + "\r\n";
    }
    req += "\r\n";
    return req;
}

static std::string makeTraversalRequest(const std::string& target)
{
    return "GET " + target + " HTTP/1.1\r\nHost: example.com\r\n\r\n";
}

static std::string makeDualContentLength(const std::string& first, const std::string& second, const std::string& body)
{
    return "POST /smuggle HTTP/1.1\r\nHost: example.com\r\n"
        "Content-Length: " + first + "\r\n"
        "Content-Length: " + second + "\r\n\r\n" + body;
}

static std::string makeDualTransferEncoding(const std::string& first, const std::string& second, const std::string& chunkedBody)
{
    return "POST /smuggle HTTP/1.1\r\nHost: example.com\r\n"
        "Transfer-Encoding: " + first + "\r\n"
        "Transfer-Encoding: " + second + "\r\n\r\n" + chunkedBody;
}

static std::string makeControlCharHeaderRequest()
{
    std::string req = "GET / HTTP/1.1\r\nHost: example.com\r\nX-Test: abc";
    req.push_back('\x01');
    req += "def\r\n\r\n";
    return req;
}

static bool verifyExpectedRequest(const Request& req, const ExpectedRequest& expected, std::string& why)
{
    if (expected.method.has_value() && req.getMethod() != *expected.method) {
        why = "method mismatch expected=" + toText(*expected.method)
            + " got=" + toText(req.getMethod());
        return false;
    }
    if (expected.path.has_value() && req.getPath() != *expected.path) {
        why = "path mismatch expected='" + *expected.path + "' got='" + req.getPath() + "'";
        return false;
    }
    if (expected.query.has_value() && req.getQuery() != *expected.query) {
        why = "query mismatch expected='" + *expected.query + "' got='" + req.getQuery() + "'";
        return false;
    }
    if (expected.version.has_value() && req.getVersion() != *expected.version) {
        why = "version mismatch expected=" + toText(*expected.version)
            + " got=" + toText(req.getVersion());
        return false;
    }
    if (expected.chunked.has_value() && req.getChunked() != *expected.chunked) {
        why = std::string("chunked mismatch expected=") + (*expected.chunked ? "true" : "false")
            + " got=" + (req.getChunked() ? "true" : "false");
        return false;
    }
    if (expected.contentLength.has_value() && req.getContentLen() != *expected.contentLength) {
        why = "content-length mismatch expected=" + std::to_string(*expected.contentLength)
            + " got=" + std::to_string(req.getContentLen());
        return false;
    }
    if (expected.body.has_value() && req.getBodyAsString() != *expected.body) {
        why = "body mismatch expected='" + *expected.body + "' got='" + req.getBodyAsString() + "'";
        return false;
    }
    return true;
}

static bool runCase(const TestCase& tc)
{
    RequestParser parser;
    std::optional<Request> finalResult;
    bool threw = false;
    std::string exceptionMsg;
    std::ostringstream capturedErr;
    std::streambuf* oldErrBuf = std::cerr.rdbuf(capturedErr.rdbuf());

    for (size_t i = 0; i < tc.chunks.size(); ++i) {
        try {
            std::optional<Request> result = parser.parseClientRequest(tc.chunks[i]);
            if (result.has_value()) {
                finalResult = result;
            }
        } catch (const std::exception& e) {
            threw = true;
            exceptionMsg = e.what();
            break;
        }
    }

    std::cerr.rdbuf(oldErrBuf);

    bool pass = false;
    std::string detail;
    if (tc.expectation == Expectation::SUCCESS) {
        pass = (!threw && finalResult.has_value());
        if (pass) {
            pass = verifyExpectedRequest(*finalResult, tc.expected, detail);
        } else {
            detail = "parser did not produce a complete request";
        }
    } else if (tc.expectation == Expectation::ERROR) {
        pass = threw || parser.getState() == ParserState::ERROR;
        if (!pass) {
            detail = "expected parse error but parser stayed in state=" + std::to_string(static_cast<int>(parser.getState()));
        }
    } else {
        pass = !threw && !finalResult.has_value() && parser.getState() != ParserState::ERROR;
        if (!pass) {
            detail = "expected incomplete parse without error";
        }
    }

    std::cout << "  [" << (pass ? "PASS" : "FAIL") << "] "
              << std::left << std::setw(38) << tc.name
              << " expected=" << toText(tc.expectation)
              << " got=";

    if (threw) {
        std::cout << "exception(" << exceptionMsg << ")";
    } else if (finalResult.has_value()) {
        std::cout << "success";
    } else {
        std::cout << "incomplete"
                  << " state=" << static_cast<int>(parser.getState());
    }

    if (!detail.empty()) {
        std::cout << " detail=" << detail;
    }

    std::cout << "\n";
    if (pass && finalResult.has_value() && tc.debugPrintHeaders) {
        std::cout << "      parsed headers:\n" << finalResult->printHeaders();
    }
    if (!pass && !capturedErr.str().empty()) {
        std::cout << "      parser_log: " << capturedErr.str() << "\n";
    }
    return pass;
}

static void runGroup(const std::string& title, const std::vector<TestCase>& tests,
                     int& totalPass, int& totalCount)
{
    std::cout << "\n============================================================\n";
    std::cout << title << "\n";
    std::cout << "============================================================\n";

    int groupPass = 0;
    for (size_t i = 0; i < tests.size(); ++i) {
        if (runCase(tests[i])) {
            groupPass++;
        }
    }

    totalPass += groupPass;
    totalCount += static_cast<int>(tests.size());
    std::cout << "Group summary: " << groupPass << "/" << tests.size() << " passed\n";
}

int main()
{
    const std::vector<TestCase> uriAndLinePass = {
        {
            "GET root",
            {"GET / HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "GET with query",
            {"GET /search?q=webserv HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/search", "q=webserv", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "GET trims fragment, stores query",
            {"GET /p/a?t=9#ignored HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/p/a", "t=9", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "GET path normalize dot-segments",
            {"GET /a/b/../c/./d HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/a/c/d", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "GET decodes unreserved percent bytes",
            {"GET /users/%7Ealice HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/users/~alice", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "GET keeps reserved slash encoded",
            {"GET /a%2Fb HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/a%2Fb", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "HTTP/1.0 without Host",
            {"GET /legacy HTTP/1.0\r\nUser-Agent: t\r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/legacy", "", HttpVersion::HTTP_1_0, false, std::nullopt, ""},
            false
        },
        {
            "Host key case-insensitive",
            {"GET / HTTP/1.1\r\nHOST: example.com\r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "Header value trimming",
            {"GET / HTTP/1.1\r\nHost:    example.com   \r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "DELETE basic",
            {"DELETE /resource/42 HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::DELETE, "/resource/42", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "Duplicate custom header (last wins)",
            {"GET /dup HTTP/1.1\r\nHost: example.com\r\nX-Test: first\r\nX-Test: second\r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/dup", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            true
        },
    };

    const std::vector<TestCase> bodyAndStreamingPass = {
        {
            "POST with content-length body",
            {"POST /api/data HTTP/1.1\r\nHost: example.com\r\nContent-Length: 24\r\n\r\n{\"name\":\"John\",\"age\":30}"},
            Expectation::SUCCESS,
            {HttpMethod::POST, "/api/data", "", HttpVersion::HTTP_1_1, false, 24, "{\"name\":\"John\",\"age\":30}"},
            false
        },
        {
            "POST with zero content-length",
            {"POST /api/empty HTTP/1.1\r\nHost: example.com\r\nContent-Length: 0\r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::POST, "/api/empty", "", HttpVersion::HTTP_1_1, false, 0, ""},
            false
        },
        {
            "Chunked body single chunk",
            {"POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nHello\r\n0\r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::POST, "/upload", "", HttpVersion::HTTP_1_1, true, std::nullopt, "Hello"},
            false
        },
        {
            "Chunked body multi chunk",
            {"POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::POST, "/upload", "", HttpVersion::HTTP_1_1, true, std::nullopt, "Wikipedia"},
            false
        },
        {
            "Chunked body with extension",
            {"POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n4;foo=bar\r\nTest\r\n0\r\n\r\n"},
            Expectation::SUCCESS,
            {HttpMethod::POST, "/upload", "", HttpVersion::HTTP_1_1, true, std::nullopt, "Test"},
            false
        },
        {
            "Incremental split across line+headers+body",
            {
                "POST /stream HTTP/1.1\r\n",
                "Host: example.com\r\nContent-Length: 5\r\n\r\n",
                "Hello"
            },
            Expectation::SUCCESS,
            {HttpMethod::POST, "/stream", "", HttpVersion::HTTP_1_1, false, 5, "Hello"},
            false
        },
        {
            "Incremental chunked body",
            {
                "POST /chunk HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n",
                "5\r\nHello\r\n",
                "0\r\n\r\n"
            },
            Expectation::SUCCESS,
            {HttpMethod::POST, "/chunk", "", HttpVersion::HTTP_1_1, true, std::nullopt, "Hello"},
            false
        },
        {
            "Incomplete waiting for header terminator",
            {"GET / HTTP/1.1\r\nHost: example.com\r\n"},
            Expectation::INCOMPLETE,
            {},
            false
        },
        {
            "Incomplete waiting for remaining body bytes",
            {"POST /x HTTP/1.1\r\nHost: example.com\r\nContent-Length: 10\r\n\r\n12345"},
            Expectation::INCOMPLETE,
            {},
            false
        },
        {
            "Incomplete request line",
            {"GET / HTTP/1.1"},
            Expectation::INCOMPLETE,
            {},
            false
        },
    };

    const std::vector<TestCase> malformedShouldFail = {
        {
            "Missing Host on HTTP/1.1",
            {"GET / HTTP/1.1\r\nUser-Agent: test\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Invalid method",
            {"PATCH / HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Lowercase method rejected",
            {"get / HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Invalid HTTP version",
            {"GET / HTTP/2.0\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Missing version token",
            {"GET /\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Target without leading slash",
            {"GET noslash HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Absolute URI with scheme",
            {"GET http://evil.com/a HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Authority form in path",
            {"GET //evil.com/a HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Userinfo-like @ in first segment",
            {"GET /user@host/path HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Path too long >255",
            {"GET /" + std::string(260, 'a') + " HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Malformed percent hex",
            {"GET /bad/%ZZ HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Truncated percent triplet",
            {"GET /bad/%A HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Encoded null byte",
            {"GET /bad/%00/path HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Malformed header no colon",
            {"GET / HTTP/1.1\r\nHost example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Header key contains spaces",
            {"GET / HTTP/1.1\r\nBad Key: x\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Unsupported transfer encoding",
            {"POST /x HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: gzip\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Transfer-Encoding and Content-Length together",
            {"POST /x HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\nContent-Length: 3\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Malformed content-length alpha",
            {"POST /x HTTP/1.1\r\nHost: example.com\r\nContent-Length: 10a\r\n\r\n0123456789"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Content-Length too large",
            {"POST /x HTTP/1.1\r\nHost: example.com\r\nContent-Length: 104857601\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Chunked invalid hex digit",
            {"POST /x HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\nZ\r\nabc\r\n0\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Chunked missing trailing CRLF",
            {"POST /x HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nWikiX\r\n0\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Chunk size over max",
            {"POST /x HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n800001\r\na\r\n0\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Single header line too long",
            {makeLongHeaderRequest(33000)},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Total headers too large",
            {makeHugeHeaderBlockRequest(340, 800)},
            Expectation::ERROR,
            {},
            false
        },
    };

    const std::vector<TestCase> traversalAndNormalizationHardening = {
        {
            "Dot-dot canonicalizes at URI level",
            {makeTraversalRequest("/../../etc/passwd")},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/etc/passwd", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "Encoded dot segments canonicalize",
            {makeTraversalRequest("/%2e%2e/%2e%2e/secret.txt")},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/secret.txt", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "Mixed encode and dot canonicalize",
            {makeTraversalRequest("/a/%2E./b/../c")},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/c", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "Encoded slash preserved in segment",
            {makeTraversalRequest("/safe/..%2f..%2fetc/passwd")},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/safe/..%2f..%2fetc/passwd", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "Double-encoded traversal remains encoded",
            {makeTraversalRequest("/%252e%252e/%252e%252e/etc/passwd")},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/%252e%252e/%252e%252e/etc/passwd", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "Backslash remains literal char",
            {makeTraversalRequest("/..\\..\\windows\\win.ini")},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/..\\..\\windows\\win.ini", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
        {
            "Overlong UTF-8 bytes stay encoded",
            {makeTraversalRequest("/%c0%ae%c0%ae/%c0%ae%c0%ae/etc")},
            Expectation::SUCCESS,
            {HttpMethod::GET, "/%c0%ae%c0%ae/%c0%ae%c0%ae/etc", "", HttpVersion::HTTP_1_1, false, std::nullopt, ""},
            false
        },
    };

    const std::vector<TestCase> smugglingAndFramingHardening = {
        {
            "Duplicate Content-Length same value",
            {makeDualContentLength("5", "5", "Hello")},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Duplicate Content-Length conflicting",
            {makeDualContentLength("5", "10", "HelloWorld")},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Duplicate Transfer-Encoding chain",
            {makeDualTransferEncoding("chunked", "chunked", "5\r\nHello\r\n0\r\n\r\n")},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Transfer-Encoding token list",
            {"POST /smuggle HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: gzip, chunked\r\n\r\n5\r\nHello\r\n0\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Chunk-size line with leading plus",
            {"POST /smuggle HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n+5\r\nHello\r\n0\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Chunk-size with internal whitespace",
            {"POST /smuggle HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n5 ;x=y\r\nHello\r\n0\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Chunked body with trailing junk",
            {"POST /smuggle HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nHello\r\n0\r\n\r\nGARBAGE"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "TE CL classic smuggling pattern",
            {"POST /smuggle HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\nContent-Length: 4\r\n\r\n4\r\nWiki\r\n0\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
    };

    const std::vector<TestCase> strictGrammarEdgeCases = {
        {
            "Request-line with tab separators",
            {"GET\t/\tHTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Request-line with multiple spaces",
            {"GET  / HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Asterisk-form unsupported",
            {"OPTIONS * HTTP/1.1\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Header obs-fold continuation",
            {"GET / HTTP/1.1\r\nHost: example.com\r\nX-Test: abc\r\n\tdef\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Header with empty key",
            {"GET / HTTP/1.1\r\n: value\r\nHost: example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Header key with space before colon",
            {"GET / HTTP/1.1\r\nHost : example.com\r\n\r\n"},
            Expectation::ERROR,
            {},
            false
        },
        {
            "Header value contains control char",
            {makeControlCharHeaderRequest()},
            Expectation::ERROR,
            {},
            false
        },
        {
            "LF-only line endings",
            {"GET / HTTP/1.1\nHost: example.com\n\n"},
            Expectation::ERROR,
            {},
            false
        },
    };

    int totalPass = 0;
    int totalCount = 0;

    std::cout << "\nHTTP Parser Regression Suite\n";
    std::cout << "------------------------------------------------------------\n";

    runGroup("URI And Request-Line Valid Cases", uriAndLinePass, totalPass, totalCount);
    runGroup("Body + Streaming (Valid + Incomplete)", bodyAndStreamingPass, totalPass, totalCount);
    runGroup("Malformed Requests Expected To Fail", malformedShouldFail, totalPass, totalCount);
    runGroup("Traversal And Normalization Hardening", traversalAndNormalizationHardening, totalPass, totalCount);
    runGroup("Smuggling And Framing Hardening", smugglingAndFramingHardening, totalPass, totalCount);
    runGroup("Strict Grammar Edge Cases", strictGrammarEdgeCases, totalPass, totalCount);

    std::cout << "\n============================================================\n";
    std::cout << "Final Summary: " << totalPass << "/" << totalCount << " checks passed\n";
    std::cout << "============================================================\n";

    return (totalPass == totalCount) ? 0 : 1;
}
