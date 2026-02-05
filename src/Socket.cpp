#include <sys/socket.h>
#include "Socket.hpp"
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <map>
#include "config.hpp"

Socket::Socket(std::pair<int, std::string> sock) : info(sock)
{
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	const int reuse = 1;
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	
	addr.sin_port = htons(info.first);
	addr.sin_family = AF_INET;
	if (info.second == "any")
		addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(socket_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1 || listen(socket_fd, 10) == -1)	
		throw std::runtime_error(strerror(errno));
	std::cout << "webserver listening on port " << info.first << '\n';
}

Socket::Socket(const Socket &other) : socket_fd(other.socket_fd), addr(other.addr){}

Socket::~Socket()
{
	close(socket_fd);
}

int Socket::get_socket_fd() { return socket_fd; };

sockaddr_in &Socket::get_addr() { return addr; };