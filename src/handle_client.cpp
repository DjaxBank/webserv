#include "Socket.hpp"
#include "functions.hpp"
#include <string>
#include <vector>
#include <sys/select.h>
#include <requestParser.hpp>
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <ctime>
#include <fcntl.h>
#include "cgi.hpp"
#include "Response.hpp"

static std::optional<Request> receive_data(int clientfd, RequestParser &parser, std::vector<Server> &servers,  std::vector<int> &keep_alive)
{
	char	buf[1024];

	ssize_t bytes_read;
	std::string request_raw;
	std::optional<Request> parsed_request;

	while(1)
	{
		if (fcntl(clientfd, F_GETFD) == -1)
			close_socket(clientfd, servers, keep_alive);
		bytes_read = recv(clientfd, buf, 1024, 0); 
		if (bytes_read <= 0)
			break;
		std::string better_buf(buf, bytes_read);
		request_raw += better_buf;
		parsed_request = parser.parseClientRequest(better_buf);
		if (parsed_request.has_value())
			break ;
	}
	std::cout << request_raw.substr(0, request_raw.find(" HTTP/")) << ' ';
	return parsed_request;
}

static Server &find_active_server(int target_fd, std::vector<Server> &servers)
{
	for (Server &serv : servers)
	{
		if (std::find(serv.sock.client_fds.begin(), serv.sock.client_fds.end(), target_fd) != serv.sock.client_fds.end())
			return serv;
	}
	throw std::runtime_error("");
}

static Route_rule &find_correct_route(Server &serv, const Request &request)
{
	std::vector<std::string> valid_routes;
	std::string uri = request.getPath();
	
	if (uri.empty())
		throw std::runtime_error("couldn't handle request");
	for (Route_rule &cur : serv.routes)
	{
		if (uri == cur.route || uri.find(cur.route + "/") == 0 || cur.route == "/")
			valid_routes.push_back(cur.route);
	}
	if (valid_routes.empty())
		throw std::runtime_error("no matching route");
	auto longest = std::max_element(valid_routes.begin(), valid_routes.end());
	for (Route_rule &cur : serv.routes)
	{
		if (*longest == cur.route)
			return cur;
	}
	throw std::runtime_error("");
}

void close_socket(int fd, std::vector<Server> &servers, std::vector<int> &keep_alive)
{
	std::cout << "Closing connection " << std::to_string(fd) << '\n';
	close(fd);
	for (Server &serv : servers)
	{
		auto it = std::find(serv.sock.client_fds.begin(), serv.sock.client_fds.end(), fd);
		if (it != serv.sock.client_fds.end())
		{
			serv.sock.client_fds.erase(it);
			break ;
		}
	}
	auto it = std::find(keep_alive.begin(), keep_alive.end(), fd);
	if (it != keep_alive.end())
		keep_alive.erase(it);
}

void handle_client(std::vector<Server> &servers, fd_set *monitored, std::vector<int> &keep_alive, std::map<pid_t, int> &cgi, char **envp)
{
	std::vector<int>	active_fds;
	static std::map<int, Request> saved_requests;

	for (Server &serv : servers)
	{
		if (FD_ISSET(serv.sock.get_socket_fd(), monitored))
		{
			timeval tv {60, 0};
			timeval recvtimeout {2, 0};
			socklen_t addr_len = sizeof(struct sockaddr_in);
			int newfd = accept(serv.sock.get_socket_fd(), reinterpret_cast <sockaddr *>(&serv.sock.get_addr()), &addr_len); // store somewhere else
			setsockopt(newfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
			setsockopt(newfd, SOL_SOCKET, SO_RCVTIMEO, &recvtimeout, sizeof(recvtimeout));
			active_fds.push_back(newfd);
			serv.sock.client_fds.push_back(newfd);
			std::cout << "new connection " << std::to_string(newfd) << '\n';
		}
	}
	for (int fd : keep_alive)
	{
		if (FD_ISSET(fd, monitored))
			active_fds.push_back(fd);
	}
	for (auto it = cgi.begin() ; it != cgi.end() ; it++)
		if (FD_ISSET(it->first, monitored))
			active_fds.push_back(it->first);
	for (int fd : active_fds)
	{
		std::optional<Request>	parsed_request;
		try
		{
			bool is_cgi = (cgi.contains(fd));
			int cgi_fd = -1;
			if (is_cgi)
			{
				cgi_fd = fd;
				fd = cgi.find(fd)->second;
			}
			Server					&config = find_active_server(fd, servers);
			RequestParser			parser;
			std::string				status;
			if (!is_cgi)
			{
				try
				{
					parsed_request = receive_data(fd, parser, servers, keep_alive);
				}
				catch (const HttpParseException& e)
				{
					std::cerr << "Parse error on fd " << fd
							<< " (status " << e.statusCode() << "): "
							<< e.what() << '\n';
							throw std::runtime_error("");
							
						}
						catch (const std::exception& e)
						{
							std::cerr << "Error handling request on fd " << fd
							<< ": " << e.what() << '\n';
							throw std::runtime_error("");
						}
						if (!parsed_request.has_value())
						{
							char buf;
							if (recv(fd, &buf, 1, 0) <= 0)
							{
								close_socket(fd, servers, keep_alive);
								continue;
							}
							std::cerr << "Failed to parse request from fd " << fd << ", skipping.\n";
							throw std::runtime_error("");
				}
			}
			Route_rule			*route;
			if (!is_cgi) 
				route = &find_correct_route(config, *parsed_request);
			try
			{
				if (is_cgi)
				{
					auto saved_request = saved_requests.find(fd);
					Response	response(&config, &saved_request->second, fd, envp, cgi_fd);
					response.Reply();
					cgi.erase(cgi.find(cgi_fd));
					saved_requests.erase(saved_request);
				}
				else
				{
					if (new_cgi(route->root + "/" + parsed_request->getPath().substr(route->route.length()), config, parsed_request->getBodyAsString(), cgi, fd, envp))
					{
						saved_requests.emplace(std::pair<int, Request>{fd, parsed_request.value()});
						continue ;
					}
					Response response(&config, route, &parsed_request.value(), fd, envp);
					response.Reply();
				}
				
			}
			catch(const std::exception& e)
			{
				std::cerr << e.what() << '\n';
				Response errorresponse(fd, "500 Internal Server Error");  
				errorresponse.Reply();
			}
			
		}
		catch(const std::exception& e)
		{
			Response errorresponse(fd, "400 Bad Request");
			errorresponse.Reply();
			close_socket(fd, servers, keep_alive);
			continue;
		}
		if (std::find(keep_alive.begin(), keep_alive.end(), fd) == keep_alive.end())
			keep_alive.push_back(fd);
		}
	}