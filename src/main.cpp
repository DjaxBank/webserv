#include <iostream>
#include <vector>
#include <sys/select.h>
#include <fstream>
#include "Socket.hpp"
#include "functions.hpp"
#include "signal.h"
#include <ostream>
#include "Server.hpp"

static bool server_running = true;

static void signal_handler(int signal)
{
	(void)signal;
	server_running = false;
	std::cout << '\n';
}

static fd_set setup_socket_fds(std::vector<Server> &servers)
{
	fd_set socket_fds;
	FD_ZERO(&socket_fds);
	for (Server &serv : servers)
			FD_SET(serv.sock.get_socket_fd(), &socket_fds);
	return socket_fds;
}

static void reset_sockets(std::vector<Server> &servers, fd_set &socket_fds, int &max_fd)
{
	socket_fds = setup_socket_fds(servers);
	max_fd = 0;
	for (Server &serv : servers)
		if (serv.sock.get_socket_fd() > max_fd)
			max_fd = serv.sock.get_socket_fd();
}

static void server_loop(std::vector<Server> servers)
{
	fd_set	socket_fds;
	int		max_fd;
	for (Server &serv : servers)
		std::cout << "Webserver listening on " << serv.sock.info.second << " interface port " <<  std::to_string(serv.sock.info.first) << '\n';
	std::cout << '\n';
	while (server_running)
	{
		timeval timeout{5, 0};
		reset_sockets(servers, socket_fds, max_fd);
		if (select(max_fd + 1, &socket_fds, NULL, NULL, &timeout) > 0)
		{
			try
			{
				handle_client(servers, &socket_fds);
			}
			catch(const std::exception& e)
			{
				std::cerr << e.what() << '\n';
			}
		}
	}
}

static std::vector<Server> importconfigfile(char *configfile)
{
	std::ifstream config(configfile);
	std::vector<Server> servers;

	while (config.is_open() && !config.eof())
	{
		std::string line;
		getline(config, line);
		if (line.find("server") != line.npos)
			servers.emplace_back(config);
		else if (!line.empty() && line.find("server") == line.npos)
			throw std::runtime_error("unexpected attribute: " + line);
	}
	return servers;
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
		server_loop(importconfigfile(argv[1]));
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}