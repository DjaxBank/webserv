#include "Socket.hpp"
#include <vector>
#include <sys/select.h>


void receive_data(std::vector<int> &client_fds)
{
	
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
}