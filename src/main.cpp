#include <iostream>
#include "config.hpp"

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "./webserv [configuration file]\n";
		return EXIT_FAILURE;
	}
	bool server_running = true;
	Config Config(argv[1]);
	while (server_running)
	{
		
	}
}