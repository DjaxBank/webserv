#include <iostream>
#include <vector>
#include "config.hpp"
#include "Socket.hpp"

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
			server_running = false;
			
			// accept(sock.get_socket_fd());
		}
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}