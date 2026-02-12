#include <sys/socket.h>
#include "Socket.hpp"
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <map>
#include <netdb.h>

void Socket::setinterface()
{
	if (info.second == "any")
		addr.sin_addr.s_addr = INADDR_ANY;
	else if (info.second == "localhost")
		addr.sin_addr.s_addr = INADDR_LOOPBACK;
	else
	{ 										// DOESN'T WORK
		struct addrinfo req;
		req.ai_family = addr.sin_family;
		req.ai_socktype = SOCK_STREAM;
		struct addrinfo *result = nullptr;
		int check = getaddrinfo(info.second.c_str(), nullptr, &req, &result);
		if (check != 0 || result == 0)
			throw std::runtime_error(gai_strerror(check));
		addr.sin_addr = reinterpret_cast<struct sockaddr_in*>(result->ai_addr)->sin_addr;
		freeaddrinfo(result);
	}
}

Socket::Socket(std::pair<int, std::string> sock) : info(sock)
{
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	const int reuse = 1;
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	
	addr.sin_port = htons(info.first);
	addr.sin_family = AF_INET;
	setinterface();

	if (bind(socket_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1 || listen(socket_fd, 10) == -1)	
		throw std::runtime_error(strerror(errno));
}

Socket::Socket(const Socket &other) : socket_fd(dup(other.socket_fd)), addr(other.addr), info(other.info){}

Socket &Socket::operator=(const Socket &other)
{
	this->info = other.info;
	this->socket_fd = dup(other.socket_fd);
	this->addr = other.addr;
	return *this;
}

Socket::~Socket()
{
	close(socket_fd);
}

int Socket::get_socket_fd() { return socket_fd; };

sockaddr_in &Socket::get_addr() { return addr; };