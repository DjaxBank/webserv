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
		parser_state = parser.fetch_data(better_buf);
		if (better_buf.find("\r\n"))
			break ;
	}
	return parser_state;
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