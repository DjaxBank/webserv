#include <sys/socket.h>
#include "Socket.hpp"
#include <unistd.h>

Socket::Socket(unsigned int port)
{
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr));
}

Socket::~Socket()
{
	close(socket_fd);
}