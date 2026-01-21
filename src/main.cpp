#include <iostream>
#include <vector>
#include <sys/select.h>
#include "config.hpp"
#include "Socket.hpp"
#include "functions.hpp"

static fd_set setup_socket_fds(std::vector<Socket> &sockets)
{
	fd_set socket_fds;
	FD_ZERO(&socket_fds);

	for (size_t i = 0 ; i < sockets.size() ; i ++)
		FD_SET(sockets[i].get_socket_fd(), &socket_fds);
	return socket_fds;
}

static std::vector<Socket> setup_sockets(const std::vector<socket_pair> &pairs)
{
	const size_t end = pairs.size();
	std::vector<Socket> sockets;
	sockets.reserve(end);
	for (size_t i = 0 ; i < end ; i++)
	{
		sockets.emplace_back(pairs[i].port);
	}
	return sockets;
}
int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "./webserv [configuration file]\n";
		return EXIT_FAILURE;
	}
	try
	{
		Config config(argv[1]);
		bool server_running = true;
		std::vector<Socket> sockets = setup_sockets(config.socket_pairs);
		while (server_running)
		{
			fd_set socket_fds = setup_socket_fds(sockets);
			int max_fd = 0;
			for (size_t i = 0 ; i < sockets.size() ; i ++)
				if (sockets[i].get_socket_fd() > max_fd)
					max_fd = sockets[i].get_socket_fd();
			size_t active_amount = select(max_fd + 1, &socket_fds, NULL, NULL, NULL);
			if (active_amount > 0)
				handle_client(active_amount, socket_fds, sockets);
			else
				std::cerr << "select error occured\n";
		}
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}