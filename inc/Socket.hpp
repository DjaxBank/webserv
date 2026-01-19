#pragma once

#include <netinet/in.h>
class Socket
{
	private:
		Socket();
		int					socket_fd;
		struct sockaddr_in	addr;
	
	public:
		Socket(unsigned int port);
		Socket(const Socket &other);
		Socket &operator=(const Socket &other);
		~Socket();
};