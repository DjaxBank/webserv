#include <iostream>
#include <vector>
#include <sys/select.h>
#include <fstream>
#include <filesystem>
#include <ostream>
#include <fcntl.h>
#include "Socket.hpp"
#include "functions.hpp"
#include "signal.h"
#include "Server.hpp"

static bool server_running = true;

static void signal_handler(int signal)
{
	(void)signal;
	server_running = false;
	std::cout << '\n';
}

static fd_set setup_socket_fds(std::vector<int> &fd_list)
{
	fd_set socket_fds;
	FD_ZERO(&socket_fds);
	for (int fd : fd_list)
		FD_SET(fd, &socket_fds);
	return socket_fds;
}

static void reset_sockets(std::vector<Server> &servers, fd_set &socket_fds, std::vector<int> keep_alive, int &max_fd)
{
	std::vector<int> fd_list;

	for (Server &serv : servers)
		fd_list.push_back(serv.sock.get_socket_fd());
	std::vector<int>::iterator it = keep_alive.begin();
	while (it != keep_alive.end())
	{
		if (fcntl(*it, F_GETFD) == -1)
		{
			std::cout << "Closing connection " << std::to_string(*it) << '\n';
			close(*it);
			keep_alive.erase(it);
		}
		else
		{
			fd_list.push_back(*it);
			it++;
		}
	}
	socket_fds = setup_socket_fds(fd_list);
	max_fd = 0;
	for (int fd : fd_list)
		if (fd > max_fd)
			max_fd = fd;
}

static void server_loop(std::vector<Server> servers, char **envp)
{
	fd_set				socket_fds;
	int					max_fd;
	std::vector<int>	keep_alive;

	for (Server &serv : servers)
		std::cout << "Webserver listening on " << serv.sock.info.second << " interface port " <<  std::to_string(serv.sock.info.first) << '\n';
	std::cout << '\n';
	while (server_running)
	{
		timeval timeout{3, 0};
		reset_sockets(servers, socket_fds, keep_alive, max_fd);
		if (select(max_fd + 1, &socket_fds, NULL, NULL, &timeout) > 0)
		{
			try
			{
				handle_client(servers, &socket_fds, keep_alive, envp);
			}
			catch(const std::exception& e)
			{
				std::cerr << e.what() << '\n';
			}
		}
	}
}

static std::vector<Server> importconfigfile(char *configfile, char **envp)
{
	std::ifstream config(configfile);
	std::vector<Server> servers;

	if (!config.is_open())
		throw std::runtime_error("File does not exist");
	while (!config.eof())
	{
		std::string line;
		getline(config, line);
		if (line.find("server") != line.npos)
			servers.emplace_back(config, envp);
		else if (!line.empty() && line.find("server") == line.npos)
			throw std::runtime_error("unexpected attribute: " + line);
	}
	return servers;
}

int main(int argc, char **argv, char **envp)
{
	if (argc != 2)
	{
		std::cerr << "./webserv [configuration file]\n";
		return EXIT_FAILURE;
	}
	signal(SIGINT, signal_handler);
	try
	{
		server_loop(importconfigfile(argv[1], envp), envp);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}