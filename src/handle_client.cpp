#include "Socket.hpp"
#include <string>
#include <vector>
#include <sys/select.h>
#include <requestParser.hpp>
#include <iostream>

static void receive_data(int clientfd)
{
	char	buf[1024];

	size_t bytes_read = 1;
	std::cout << "new request from :" << "???" << "\n\n";
	RequestParser parser;
	size_t i = 0;
	while(bytes_read > 0)
	{
		bytes_read = recv(clientfd, buf, 1024, 0);
		std::string better_buf(buf, bytes_read);
		if (better_buf.find("\r\n"))
			bytes_read = 0;
		std::cout << i++ << better_buf << '\n';
		parser.fetch_data(better_buf);
	}
}

void handle_client(fd_set *socket_fds, std::vector<Socket> &sockets)
{
	std::vector<Socket> active_sockets;
	std::vector<int>	client_fds;

	for (size_t i = 0 ; i < sockets.size() ; i ++)
		if (FD_ISSET(sockets[i].get_socket_fd(), socket_fds))
			active_sockets.push_back(sockets[i]);
	for (size_t i = 0 ; i < active_sockets.size() ; i ++)
	{
		socklen_t addr_len = sizeof(struct sockaddr_in);
		client_fds.push_back(accept(active_sockets[i].get_socket_fd(), reinterpret_cast <sockaddr *>(&active_sockets[i].get_addr()), &addr_len));
	}
	for (int &fd : client_fds)
		receive_data(fd);
}