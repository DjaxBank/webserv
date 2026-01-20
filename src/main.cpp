#include <iostream>
#include "config.hpp"
#include "Socket.hpp"
int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "./webserv [configuration file]\n";
		return EXIT_FAILURE;
	}
	try
	{
		Config Config(argv[1]);
		bool server_running = true;
		while (server_running)
		{
			server_running = false;
			Socket sock(8080);
			// accept(sock.get_socket_fd());
		}
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}