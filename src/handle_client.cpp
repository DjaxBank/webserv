#include "Socket.hpp"
#include "functions.hpp"
#include <string>
#include <vector>
#include <sys/select.h>
#include <requestParser.hpp>
#include <iostream>
#include <unistd.h>
#include "Response.hpp"

static bool receive_data(int clientfd, RequestParser &parser)
{
	char	buf[1024];

	ssize_t bytes_read = 1;
	std::string request_raw;
	while(bytes_read > 0)
	{
		bytes_read = recv(clientfd, buf, 1024, 0);
		std::string better_buf(buf, bytes_read);
		request_raw += better_buf;
		if (request_raw.find("\r\n"))
			break ;
	}
	std::string logging = request_raw.substr(0, request_raw.find(" HTTP/"));
	std::cout << logging << ' ';
	return parser.parseClientRequest(request_raw);
}

Server &find_active_server(int target_fd, std::vector<Server> &servers)
{
	for (Server &serv : servers)
	{
		if (target_fd == serv.sock.client_fd)
			return serv;
	}
	throw std::runtime_error("");
}

void handle_client(std::vector<Server> &servers, fd_set *socket_fds)
{
	std::vector<int>	client_fds;

	for (Server &serv : servers)
	{
		serv.sock.client_fd = -1;
		if (FD_ISSET(serv.sock.get_socket_fd(), socket_fds))
		{
			socklen_t addr_len = sizeof(struct sockaddr_in);
			serv.sock.client_fd = accept(serv.sock.get_socket_fd(), reinterpret_cast <sockaddr *>(&serv.sock.get_addr()), &addr_len);
			client_fds.push_back(serv.sock.client_fd);
		}
	}
	for (const int fd : client_fds)
	{
		Server &config = find_active_server(fd, servers);
		RequestParser		parser;
		if (!receive_data(fd, parser))
		{
			Response	response(config, config.routes[0], parser, fd, "400 Bad Request");
			response.Reply();
		}
		else
		{
			Response	response(config, config.routes[0], parser, fd);
			response.Reply();
		}
	}
	for (const int fd : client_fds)
		close(fd);
}