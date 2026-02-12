#pragma once
 
#include <netinet/in.h>
#include <exception>
#include <map>
#include <string>

class Socket
{
	private:
		void				setinterface();
		int					socket_fd;
		struct sockaddr_in	addr;
	public:
		std::pair<int, std::string> info;
		int client_fd;
		Socket() : socket_fd(-1){};
		Socket(std::pair<int, std::string> sock);
		Socket(const Socket &other);
		Socket &operator=(const Socket &other);
		~Socket();
		int get_socket_fd();
		sockaddr_in &get_addr();
};