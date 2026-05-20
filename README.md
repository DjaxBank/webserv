*This project has been created as part of the 42 curriculum by <login1>[, <login2>[, <login3>[...]]].*

# Webserv

## Description

**Webserv** is a custom HTTP server written in C++ as part of the 42 curriculum.  
The goal of the project is to understand how web servers work internally by recreating the core behavior of a real HTTP server such as NGINX.

The server handles HTTP requests, manages client connections, serves static content, executes CGI scripts, and supports configurable virtual servers through a custom configuration file.

This project focuses on:
- Socket programming
- Non-blocking I/O
- HTTP protocol implementation
- Event-driven server architecture
- Process and resource management
- Configuration parsing

The objective is to build a stable, efficient, and standards-compliant web server from scratch without relying on existing HTTP server libraries.

---

## Features

Depending on the implementation, the server may support:

- HTTP/1.1 request handling
- Multiple virtual servers
- Configurable host and port
- GET, POST, and DELETE methods
- Static file serving
- CGI execution (PHP, Python, etc.)
- File upload support
- Directory listing (autoindex)
- Custom error pages
- Request body parsing
- Redirections
- Non-blocking sockets with `poll()`, `select()`, `epoll()`, or `kqueue()`
- Connection timeout handling
- Configuration file parsing similar to NGINX

---

## Project Structure

```bash
.
├── Makefile
├── README.md
├── include/
├── src/
├── config/
├── www/
└── tests/
```

---

## Instructions

### Requirements

- C++98 compiler
- GNU Make
- Unix-based operating system (Linux/macOS)

### Compilation

Clone the repository and compile the project:

```bash
git clone <repository_url>
cd webserv
make
```

Available Makefile rules:

```bash
make        # Build the project
make clean  # Remove object files
make fclean # Remove object files and executable
make re     # Rebuild the project
```

---

## Execution

Run the server with a configuration file:

```bash
./webserv config/default.conf
```

If no configuration file is provided, the server may use a default configuration.

Example:

```bash
curl http://localhost:8080/
```

---

## Configuration

The server uses a custom configuration format inspired by NGINX.

Example:

```conf
server {
    listen 8080;
    server_name localhost;

    root ./www;
    index index.html;

    location /uploads {
        upload_enable on;
    }

    error_page 404 ./errors/404.html;
}
```

---

## HTTP Features

### Supported Methods

| Method | Description |
|--------|-------------|
| GET | Retrieve resources |
| POST | Send data to the server |
| DELETE | Remove resources |

### CGI Support

The server may execute CGI scripts such as:

- PHP
- Python
- Bash

Example:

```bash
http://localhost:8080/cgi-bin/script.py
```

---

## Testing

Useful tools for testing:

```bash
curl
telnet
nc
siege
ab
```

Example requests:

```bash
curl -X GET http://localhost:8080/

curl -X POST -d "hello=world" http://localhost:8080/

curl -X DELETE http://localhost:8080/file.txt
```

Browser testing is also recommended.

---

## Technical Choices

### Event Management

The server uses non-blocking sockets to handle multiple clients simultaneously without creating one thread per connection.

Possible system calls used:
- `socket`
- `bind`
- `listen`
- `accept`
- `poll`
- `select`
- `epoll`
- `kqueue`

### Parsing

The HTTP parser processes:
- Request line
- Headers
- Body
- Chunked transfer encoding (if implemented)

### Error Handling

The server returns proper HTTP status codes and custom error pages when configured.

---

## Resources

### HTTP Documentation

- https://developer.mozilla.org/en-US/docs/Web/HTTP
- https://datatracker.ietf.org/doc/html/rfc7230
- https://beej.us/guide/bgnet/
- https://nginx.org/en/docs/
- https://man7.org/linux/man-pages/

### CGI Documentation

- https://datatracker.ietf.org/doc/html/rfc3875

### AI Usage

AI tools were used during the development of this project for:
- Understanding HTTP protocol behavior
- Clarifying socket programming concepts
- Explaining CGI execution flow
- Reviewing architecture ideas
- Debugging specific implementation issues
- Improving documentation and README structure

AI-generated code was reviewed, tested, and adapted before integration into the project.

---

## Learning Outcomes

Through this project, we learned:
- How web servers operate internally
- The fundamentals of network programming
- Event-driven architecture design
- HTTP protocol details
- Process and connection management
- Writing robust low-level systems in C++

---

## Authors

- <login1>
- <login2>
- <login3>
