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
#include <filesystem>
#include <fcntl.h>
#include "cgi.hpp"
#include "Response.hpp"

static std::optional<Request> receive_data(int clientfd, RequestParser &parser)
{
	char	buf[1024];

	ssize_t bytes_read;
	std::optional<Request> parsed_request;

	while(1)
	{
		bytes_read = recv(clientfd, buf, 1024, 0); 
		if (bytes_read <= 0)
			break;
		std::string better_buf(buf, bytes_read);
		parsed_request = parser.parseClientRequest(better_buf);
		if (parsed_request.has_value())
			break ;
	}
	return parsed_request;
}

static Server *find_active_server(int target_fd, std::vector<Server> &servers)
{
	for (Server &serv : servers)
	{
		if (serv.sock.client_fds.contains(target_fd))
			return &serv;
	}
	return nullptr;
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
	std::vector<std::string>::iterator longest = std::max_element(valid_routes.begin(), valid_routes.end());
	for (Route_rule &cur : serv.routes)
	{
		if (*longest == cur.route)
			return cur;
	}
	std::abort();
}

void close_socket(int fd, std::vector<Server> &servers, std::vector<int> &keep_alive)
{
	std::cout << "Closing connection " << std::to_string(fd) << '\n';
	close(fd);
	for (Server &serv : servers)
	{
		if (serv.sock.client_fds.contains(fd))
		{
			serv.sock.client_fds.erase(serv.sock.client_fds.find(fd));
			break ;
		}
	}
	std::vector<int>::iterator it = std::find(keep_alive.begin(), keep_alive.end(), fd);
	if (it != keep_alive.end())
		keep_alive.erase(it);
}

static bool MethodAllowed(Route_rule &route, HttpMethod method)
{
	for (HttpMethod cur : route.http_methods)
		if (cur == method)
			return true;
	return false;
}

void handle_client(std::vector<Server> &servers, fd_set *monitored, std::vector<int> &keep_alive, std::vector<t_cgi> &cgi, char **envp)
{
	std::vector<int>	active_fds;
	static std::map<int, Request> saved_requests;
	static std::map<int, Server> saved_configs;

	for (Server &serv : servers)
	{
		if (FD_ISSET(serv.sock.get_socket_fd(), monitored))
		{
			timeval tv {60, 0};
			timeval recvtimeout {1, 0};
			socklen_t addr_len = sizeof(struct sockaddr_in);
			int newfd = accept(serv.sock.get_socket_fd(), reinterpret_cast <sockaddr *>(&serv.sock.get_addr()), &addr_len);
			setsockopt(newfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
			setsockopt(newfd, SOL_SOCKET, SO_RCVTIMEO, &recvtimeout, sizeof(recvtimeout));
			serv.sock.client_fds.emplace((std::pair<int, std::string>){newfd, serv.sock.get_ip()});
			if (std::find(keep_alive.begin(), keep_alive.end(), newfd) == keep_alive.end())
				keep_alive.push_back(newfd);
			std::cout << "new connection " << std::to_string(newfd) << '\n';
		}
	}
	for (int fd : keep_alive)
	{
		if (FD_ISSET(fd, monitored))
			active_fds.push_back(fd);
	}
	for (t_cgi &cur : cgi)
		if (FD_ISSET(cur.pipe, monitored))
			active_fds.push_back(cur.pipe);
	for (int fd : active_fds)
	{
		std::optional<Request>	parsed_request;
		Server					*config = find_active_server(fd, servers);
		try
		{
			bool is_cgi = find_cgi(cgi, fd) != nullptr;
			int cgi_fd;
			if (is_cgi)
			{
				cgi_fd = fd;
				fd = find_cgi(cgi, fd)->sock;
			}
			RequestParser			parser;
			std::string				status;
			if (!is_cgi)
			{
				try
				{
					parsed_request = receive_data(fd, parser);
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
							close_socket(fd, servers, keep_alive);
							continue;
						}
			}
			Route_rule			*route;
			if (!is_cgi) 
				route = &find_correct_route(*config, *parsed_request);
			try
			{
				if (is_cgi)
				{
					std::map<int, Server>::iterator saved_config = saved_configs.find(fd);
					std::map<int, Request>::iterator saved_request = saved_requests.find(fd);
					Response	response(&saved_config->second, &saved_request->second, fd, envp, cgi_fd);
					response.Reply();
					for (auto it = cgi.begin() ; it != cgi.end() ; it++)
					{
						if (it->pipe == cgi_fd)
						{
							cgi.erase(it);
							break ;
						}
					}
					saved_configs.erase(saved_config);
					saved_requests.erase(saved_request);
				}
				else
				{
					if (!MethodAllowed(*route, parsed_request.value().getMethod()))
					{
						Response response(fd, config, &parsed_request.value(), "405 Method Not Allowed");
						response.Reply();
						continue ;
					}
					std::string filelocation(route->root + "/" + parsed_request->getPath().substr(route->route.length()));
					if (std::filesystem::exists(filelocation) && new_cgi(filelocation, *config, *parsed_request, cgi, fd, envp))
					{
						saved_requests.emplace(std::pair<int, Request>{fd, parsed_request.value()});
						saved_configs.emplace(std::pair<int, Server>{fd, *config});
						continue ;
					}
					Response response(config, route, &parsed_request.value(), fd, envp);
					response.Reply();
				}
				
			}
			catch(const std::exception& e)
			{
				std::cerr << e.what() << '\n';
				Response errorresponse(fd, config, &parsed_request.value(), "500 Internal Server Error");  
				errorresponse.Reply();
			}
			
		}
		catch(const std::exception& e)
		{
			Response errorresponse(fd, config, &parsed_request.value(), "400 Bad Request");
			errorresponse.Reply();
			close_socket(fd, servers, keep_alive);
			continue;
		}
	}
}