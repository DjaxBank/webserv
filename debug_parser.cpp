#include "inc/requestParser.hpp"
#include <iostream>
#include <string>
#include <algorithm>

// Forward declare external functions from requestParser.cpp
extern std::string state_tostring(ParserState state);

int main() {
    RequestParser parser;

    std::string mock_request = "GET /index.html HTTP/1.1\r\n"
                            "Host: localhost:8080\r\n"
                            "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36\r\n"
                            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"
                            "Accept-Language: en-US,en;q=0.9\r\n"
                            "Accept-Encoding: gzip, deflate\r\n"
                            "Connection: keep-alive\r\n"
                            "Upgrade-Insecure-Requests: 1\r\n"
                            "Transfer-Encoding: chunked\r\n"
                            "\r\n"
                            "5\r\n"
                            "hello\r\n"
                            "0\r\n"
                            "\r\n";

    
    // Simulate socket recv() by feeding data in chunks

    // lookup header multiple values

    // figure our why chunk size changes breaks it
    size_t chunk_size = 50;
    size_t offset = 0;
    
    std::cout << "[DEBUG] Starting parser with mock request\n";
    std::cout << "[DEBUG] Total request size: " << mock_request.size() << " bytes\n\n";
    
    while (offset < mock_request.size())
    {
        size_t bytes_to_read = std::min(chunk_size, mock_request.size() - offset);
        std::string chunk = mock_request.substr(offset, bytes_to_read);
        offset += bytes_to_read;
        
        // std::cout << ">> Recv chunk [" << bytes_to_read << " bytes]: " 
        //           << (chunk.size() > 30 ? chunk.substr(0, 30) + "..." : chunk) << "\n";
        
        if (!parser.parseClientRequest(chunk))
        {
            std::cerr << "[ERROR] Parse failed at offset " << offset << "\n";
            parser.debugState("AFTER_FAILURE");
            return 1;
        }
        
        std::cout << "   State after chunk: " << (parser.getState() == ParserState::REQUEST_LINE ? "REQUEST_LINE" :
                                                   parser.getState() == ParserState::HEADERS ? "HEADERS" :
                                                   parser.getState() == ParserState::BODY ? "BODY" :
                                                   parser.getState() == ParserState::COMPLETE ? "COMPLETE" : "ERROR") << "\n\n";
    }
 
    // parser.debugState("FINAL_STATE");
    
    if (parser.getState() == ParserState::COMPLETE)
    {
        std::cout << "\n[SUCCESS] Parsing completed successfully\n";
        return 0;
    }
    else
    {   
        std::cerr << "\n[ERROR] Parsing incomplete. State: " 
                  << (parser.getState() == ParserState::BODY ? "BODY" : "OTHER") << "\n";
        parser.debugState("AFTER_INCOMPLETE");
        return 1;
    }
}
