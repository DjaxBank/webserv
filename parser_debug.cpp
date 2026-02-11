#include <iostream>
#include <string>
#include "requestParser.hpp"

int main() {
    RequestParser parser;

    // Example HTTP requests to test
    std::string requests[] = {
        "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n",
        "POST /submit?name=foo HTTP/1.1\r\nHost: example.com\r\nContent-Length: 5\r\n\r\nhello",
        "GET /docs/../guide/intro.html?version=2 HTTP/1.1\r\nHost: example.com\r\n\r\n",
        "GET /my%20file.html HTTP/1.1\r\nHost: example.com\r\n\r\n",
        "GET /../../../etc/passwd HTTP/1.1\r\nHost: example.com\r\n\r\n"
    };

    for (size_t i = 0; i < sizeof(requests)/sizeof(requests[0]); ++i) {
        std::cout << "\n--- Test Request " << (i+1) << " ---" << std::endl;
        bool result = parser.parseClientRequest(requests[i]);
        parser.debugState("Test Request");
        if (!result) {
            std::cout << "Parser error or incomplete." << std::endl;
        } else {
            std::cout << "Parser completed successfully." << std::endl;
        }
    }

    return 0;
}
