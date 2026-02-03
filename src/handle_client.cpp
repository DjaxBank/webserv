#include "Socket.hpp"
#include "functions.hpp"
#include <string>
#include <vector>
#include <sys/select.h>
#include <requestParser.hpp>
#include <iostream>
#include <unistd.h>

static bool receive_data(int clientfd, RequestParser &parser)
{
	char	buf[1024];
	bool	parser_state;

	ssize_t bytes_read = 1;
	std::cout << '\n';
	while(bytes_read > 0)
	{
		bytes_read = recv(clientfd, buf, 1024, 0);
		std::string better_buf(buf, bytes_read);
		std::cout << better_buf << '\n';
		parser_state = parser.parseClientRequest(better_buf);
		if (better_buf.find("\r\n"))
			break ;
	}
	return parser_state;
}

static void check_ready(std::vector<int> &client_fds)
{
	fd_set set;
	FD_ZERO(&set);
	int max_fd = 0;
	for (const int fd : client_fds)
	{
		FD_SET(fd, &set);
		if (fd > max_fd)
			max_fd = fd;
	}

	timeval timeout{1, 0};
	select(max_fd + 1, &set, NULL, NULL, &timeout);
	for (std::vector<int>::iterator i = client_fds.begin() ; i < client_fds.end() ; i++)
	{
		if (!FD_ISSET(*i, &set))
		{
			close(*i);
			client_fds.erase(i);
		}
	}
}

void handle_client(const Config &config, fd_set *socket_fds, std::vector<Socket> &sockets)
{
	std::vector<int>	client_fds;
	
	for (size_t i = 0 ; i < sockets.size() ; i ++)
	{
		if (FD_ISSET(sockets[i].get_socket_fd(), socket_fds))
		{
			socklen_t addr_len = sizeof(struct sockaddr_in);
			client_fds.push_back(accept(sockets[i].get_socket_fd(), reinterpret_cast <sockaddr *>(&sockets[i].get_addr()), &addr_len));
		}
	}
	check_ready(client_fds);
	for (const int fd : client_fds)
	{
		RequestParser		parser;
		if (!receive_data(fd, parser))
			(1);//ServerError();
		else
			switch (parser.getMethod())
			{
				case HttpMethod::GET:
					Http_Get(fd, config.routes[0], parser);
					break;
				case (HttpMethod::POST):
					break ;
				case (HttpMethod::DELETE):

					break ;
				default:
					break;
			}
	}
	for (const int fd : client_fds)
		close(fd);
}