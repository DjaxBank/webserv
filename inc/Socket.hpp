#pragma once

#include <netinet/in.h>
#include <exception>
class Socket
{
	private:
		Socket();
		Socket &operator=(const Socket &other);
		int					socket_fd;
		struct sockaddr_in	addr;
	
	public:
		Socket(unsigned int port);
		Socket(const Socket &other);
		~Socket();
		int get_socket_fd();
		sockaddr_in &get_addr();
};