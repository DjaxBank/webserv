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
		std::map<int, std::string> client_fds;
		Socket() : socket_fd(-1){};
		Socket(std::pair<int, std::string> sock);
		Socket(const Socket &other);
		Socket &operator=(const Socket &other);
		~Socket();
		int get_socket_fd();
		std::string get_ip();
		sockaddr_in &get_addr();
};