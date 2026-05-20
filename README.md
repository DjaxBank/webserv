*This project has been created as part of the 42 curriculum by showard, dbank.*

## Description

This project implements a custom HTTP webserver written in C++. The goal is to build a fully functional webserver from scratch, handling HTTP requests and responses, socket programming, and CGI script execution. The server can parse HTTP requests, serve static files, execute PHP scripts through CGI, and manage multiple client connections.

### Key Features

- **HTTP Protocol Support**: Implements core HTTP/1.1 protocol for handling client requests and responses
- **Socket Programming**: Uses low-level socket API for network communication
- **Request Parsing**: Sophisticated parsing of HTTP request lines, headers, and message bodies
- **CGI Support**: Executes PHP and other CGI scripts with proper environment variable setup
- **Configuration-Based**: Reads server configuration file for flexible deployment
- **Static File Serving**: Serves HTML, CSS, and other static assets from the pages directory
- **Error Handling**: Comprehensive exception handling and error responses

## Instructions

### Compilation

To compile the project, use the provided Makefile:

make

This will compile all source files in the `src/` and `parsing/` directories and generate an executable in the root of the repository.

### Configuration

Edit example.conf to configure your server settings before running. The configuration file controls server behavior such as port, listen address, and routing.

### Execution

Run the compiled server:

./webserv [config]


You can also use the provided `siege.txt` for load testing the server.

## Resources

### HTTP Protocol
- [RFC 7230 - HTTP/1.1 Message Syntax and Routing](https://tools.ietf.org/html/rfc7230)
- [RFC 7231 - HTTP/1.1 Semantics and Content](https://tools.ietf.org/html/rfc7231)

### CGI
- [RFC 3875 - The Common Gateway Interface (CGI)](https://tools.ietf.org/html/rfc3875)


## AI Usage

AI was utilized in this project for:
- **Concept Understanding**: Helping clarify HTTP protocol concepts, socket programming fundamentals, and CGI implementation details.
- **Documentation**: Assisting with the creation of this README file.

AI was not used for the core implementation of the webserver logic, request/response parsing, or critical server functionality.
