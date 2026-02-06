#include <iostream>
#include <vector>
#include <sys/select.h>
#include "config.hpp"
#include "Socket.hpp"
#include "functions.hpp"
#include "signal.h"

static bool server_running = true;

static void signal_handler(int signal)
{
	(void)signal;
	server_running = false;
	std::cout << '\n';
}

static fd_set setup_socket_fds(std::vector<Socket> &sockets)
{
	fd_set socket_fds;
	FD_ZERO(&socket_fds);

	for (size_t i = 0 ; i < sockets.size() ; i ++)
		FD_SET(sockets[i].get_socket_fd(), &socket_fds);
	return socket_fds;
}

static void reset_sockets(Config &config, fd_set &socket_fds, int &max_fd)
{
	socket_fds = setup_socket_fds(config.sockets);
	max_fd = 0;
	for (size_t i = 0 ; i < config.sockets.size() ; i ++)
		if (config.sockets[i].get_socket_fd() > max_fd)
			max_fd = config.sockets[i].get_socket_fd();
}

static void server_loop(Config config)
{
	fd_set	socket_fds;
	int		max_fd;
	for (Socket &socket : config.sockets)
		std::cout << "Webserver listening on " << socket.info.second << " interface port " <<  std::to_string(socket.info.first) << '\n';
	std::cout << '\n';
	while (server_running)
	{
		timeval timeout{1, 0};
		reset_sockets(config, socket_fds, max_fd);
		if (select(max_fd + 1, &socket_fds, NULL, NULL, &timeout) > 0)
			handle_client(config, &socket_fds, config.sockets);
	}
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "./webserv [configuration file]\n";
		return EXIT_FAILURE;
	}
	signal(SIGINT, signal_handler);
	try
	{
		server_loop(argv[1]);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}