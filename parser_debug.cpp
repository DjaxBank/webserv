#include <iostream>
#include <string>
#include "requestParser.hpp"

int main() {
    RequestParser parser;

    // Example HTTP requests to test
    std::string requests[] = {
        "GET username:password@example.com/path HTTP/1.1\r\nHost: example.com\r\n\r\n",
    };

    for (size_t i = 0; i < sizeof(requests)/sizeof(requests[0]); ++i) {
        std::cout << "\n--- Test Request " << (i+1) << " ---" << std::endl;
        std::optional<Request> result = parser.parseClientRequest(requests[i]);
        parser.debugState("Test Request");
        if (!result.has_value()) {
            std::cout << "Parser error or incomplete." << std::endl;
        } else {
            std::cout << "Parser completed successfully." << std::endl;
        }
    }

    return 0;
}
