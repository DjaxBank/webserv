#pragma once

#include <netinet/in.h>
#include <exception>
#include <map>
#include <string>

class Socket
{
	private:
		Socket();
		Socket &operator=(const Socket &other);
		int					socket_fd;
		struct sockaddr_in	addr;
		std::pair<int, std::string> info;

	public:
		Socket(std::pair<int, std::string> sock);
		Socket(const Socket &other);
		~Socket();
		int get_socket_fd();
		sockaddr_in &get_addr();
};