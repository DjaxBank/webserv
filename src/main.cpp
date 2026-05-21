

#include <iostream>
#include <vector>
#include <sys/select.h>
#include <fstream>
#include <filesystem>
#include <ostream>
#include <fcntl.h>
#include "cgi.hpp"
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

static void setup_socket_fds(std::vector<int> &read_list, fd_set &read_fds, std::vector<int> &to_respond, fd_set &write_fds)
{
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	for (int fd : read_list)
		FD_SET(fd, &read_fds);
	for (int fd : to_respond)
		FD_SET(fd, &write_fds);
}

static void reset_sockets(std::vector<Server> &servers, fd_set &read_fds, fd_set &write_fds, std::vector<int> &keep_alive,  std::vector<t_cgi> &cgi, int &max_fd, std::vector<int> &to_respond)
{
	std::vector<int>	read_list;

	for (Server &serv : servers)
		read_list.push_back(serv.sock.get_socket_fd());
	for (std::vector<int>::iterator it = keep_alive.begin(); it != keep_alive.end() ; it++)
		read_list.push_back(*it);
	for (t_cgi &cur : cgi)
		read_list.push_back(cur.pipe);
	setup_socket_fds(read_list, read_fds, to_respond, write_fds);
	max_fd = 0;
	for (int fd : read_list)
		if (fd > max_fd)
			max_fd = fd;
	for (int fd : to_respond)
		if (fd > max_fd)
			max_fd = fd;
}

static void server_loop(std::vector<Server> servers, char **envp)
{
	fd_set					read_fds;
	fd_set					write_fds;
	int						max_fd;
	std::vector<int>		keep_alive;
	std::vector<int>		to_respond;
	std::vector<t_cgi>		cgi;

	for (Server &serv : servers)
		std::cout << "Webserver listening on " << serv.sock.info.second << " interface port " <<  std::to_string(serv.sock.info.first) << '\n';
	std::cout << '\n';
	while (server_running)
	{
		if (!cgi.empty())
			check_timeout(cgi);
		timeval timeout{3, 0};
		reset_sockets(servers, read_fds, write_fds, keep_alive, cgi, max_fd, to_respond);
		if (select(max_fd + 1, &read_fds, &write_fds, NULL, &timeout) > 0)
		{
			try
			{
				handle_client(servers, &read_fds, &write_fds, keep_alive, cgi, envp, to_respond);
			}
			catch(const std::exception& e)
			{
				std::cerr << e.what() << '\n';
			}
		}
	}
}

// importconfigfile(configfile, envp)
// - Input: path to a configuration file and process environment.
// - Output: vector of Server objects parsed from the file.
// - Flow:
//   1. Open the file and fail immediately if it cannot be opened.
//   2. Scan line by line looking for top-level `server` tokens.
//   3. Construct a Server using the same ifstream so the Server parser can consume its block.
//   4. Throw on unexpected top-level text.
// - Failure cases to test/document:
//   * missing file
//   * stray text at top level
//   * false positives from substring matching on the word `server`
//   * multiple server blocks in one file

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