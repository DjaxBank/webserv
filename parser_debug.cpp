#include <iostream>
#include <string>
#include "requestParser.hpp"

int main() {
    RequestParser parser;

    // Example HTTP requests to test
    std::string requests[] = {
    "POST /api/data HTTP/1.1\r\nHost: example.com\r\nContent-Length: 24\r\n\r\n{\"name\":\"John\",\"age\":30}",
};

    for (size_t i = 0; i < sizeof(requests)/sizeof(requests[0]); ++i) {
        std::cout << "\n--- Test Request " << (i+1) << " ---" << std::endl;

        std::optional<Request> result;

        try {
            result = parser.parseClientRequest(requests[i]);
            parser.debugState("After parsing");
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        }

        if (!result.has_value()) {
            std::cout << "Parser error or incomplete." << std::endl;
        } else {
            std::cout << "Parser completed successfully." << std::endl;
        }
    }

    return 0;
}
