#include "Socket.hpp"
#include "functions.hpp"
#include <string>
#include <vector>
#include <sys/select.h>
#include <sys/wait.h>
#include <requestParser.hpp>
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <cerrno>
#include <filesystem>
#include <fcntl.h>
#include "cgi.hpp"
#include "Response.hpp"

static std::optional<Request> receive_data(int clientfd, RequestParser &parser, std::vector<Server> &servers, std::vector<int> &keep_alive)
{
	char	buf[1024];

	ssize_t					bytes_read;
	std::optional<Request>	parsed_request;

	while(1)
	{
		bytes_read = recv(clientfd, buf, 1024, 0); 
		if (bytes_read == 0)
			break;
		else if (bytes_read == -1)
		{
			close_socket(clientfd, servers, keep_alive);
			break ;
		}
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

static Route_rule &find_correct_route(Server *serv, const Request &request)
{
	std::vector<std::string> valid_routes;
	std::string uri = request.getPath();
	
	if (uri.empty())
		throw std::runtime_error("couldn't handle request");
	for (Route_rule &cur : serv->routes)
	{
		if (uri == cur.route || uri.find(cur.route + "/") == 0 || cur.route == "/")
			valid_routes.push_back(cur.route);
	}
	if (valid_routes.empty())
		throw std::runtime_error("no matching route");
	std::vector<std::string>::iterator longest = std::max_element(valid_routes.begin(), valid_routes.end());
	for (Route_rule &cur : serv->routes)
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
			serv.sock.client_fds.erase(serv.sock.client_fds.find(fd)->first);
			break ;
		}
	}
	std::vector<int>::iterator it = std::find(keep_alive.begin(), keep_alive.end(), fd);
	if (it != keep_alive.end())
		keep_alive.erase(it);
}

// if specific parse error we handle it
// else on generic error we send internal server error
// should_close is used to determine if the connection should be closed after the response is sent
// if not and the connection is not in the keep-alive list, add it to the list

static std::vector<int> setup_active(std::vector<Server> &servers, fd_set *read_fds, fd_set *write_fds, std::vector<int> &keep_alive, std::vector<int> &to_respond, std::vector<t_cgi> &cgi)
{
	std::vector<int> active_fds;
	for (Server &serv : servers)
	{
		if (FD_ISSET(serv.sock.get_socket_fd(), read_fds))
		{
			timeval tv {60, 0};
			socklen_t addr_len = sizeof(struct sockaddr_in);
			int newfd = accept(serv.sock.get_socket_fd(), reinterpret_cast <sockaddr *>(&serv.sock.get_addr()), &addr_len);
			if (newfd == -1)
			{
				std::cerr << "accept failed: " << strerror(errno) << '\n';
				continue;
			}
			setsockopt(newfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
			serv.sock.client_fds.emplace((std::pair<int, std::string>){newfd, serv.sock.get_ip()});
			if (std::find(keep_alive.begin(), keep_alive.end(), newfd) == keep_alive.end())
				keep_alive.push_back(newfd);
			std::cout << "new connection " << std::to_string(newfd) << '\n';
		}
	}
	for (t_cgi &cur : cgi)
		if (FD_ISSET(cur.pipe, read_fds))
			active_fds.push_back(cur.pipe);
	for (int fd : keep_alive)
		if (FD_ISSET(fd, read_fds))
			active_fds.push_back(fd);
	for (int fd : to_respond)
		if (FD_ISSET(fd, write_fds))
			active_fds.push_back(fd);
	return active_fds;
}

void execute_cgi(int fd, std::map<int, Request> &saved_requests, std::map<int, Server> &saved_configs, std::map<int, Route_rule> &saved_routes, std::vector<t_cgi> &cgi, char **envp, int cgi_fd, std::map<std::string, int> &cookies)
{
		std::map<int, Request>::iterator	saved_request	= saved_requests.find(fd);
		std::map<int, Server>::iterator		saved_config	= saved_configs.find(fd);
		std::map<int, Route_rule>::iterator	saved_route		= saved_routes.find(fd);
		if (find_cgi(cgi, fd)->active)
		{
			Response	response(&saved_config->second, &saved_route->second, &saved_request->second, fd, envp, cgi_fd, cookies);
			response.Reply();
		}
		else
		{
			Response timeoutresponse(fd, &saved_config->second, ReplyStatus::RequestTimeout, cookies);  
			timeoutresponse.Reply();
		}
		for (auto it = cgi.begin() ; it != cgi.end() ; it++)
		{
			if (it->pipe == cgi_fd)
			{
				waitpid(it->pid, NULL, WNOHANG);
				cgi.erase(it);
				break ;
			}
		}
		saved_configs.erase(saved_config);
		saved_requests.erase(saved_request);
		saved_routes.erase(saved_route);
}

bool is_response(int fd, std::vector<int>& to_respond)
{
	for (auto it = to_respond.begin() ; it != to_respond.end() ; it++)
	{
		if (*it == fd)
		{
			to_respond.erase(it);
			return true;
		}
	}
	return false;
}

void handle_client(std::vector<Server> &servers, fd_set *read_fds, fd_set *write_fds, std::vector<int> &keep_alive, std::vector<t_cgi> &cgi, char **envp, std::vector<int> &to_respond)
{
	std::vector<int>					active_fds = setup_active(servers, read_fds, write_fds, keep_alive, to_respond, cgi);
	static std::map<int, Request>		saved_requests;
	static std::map<int, Server>		saved_configs;
	static std::map<int, Route_rule>	saved_routes;
	static std::map<std::string, int>	cookies;
	static std::map<int, Response>		saved_responses;

	for (int fd : active_fds)
	{

		if (is_response(fd, to_respond))
		{
			auto it = saved_responses.find(fd);
			it->second.Reply();
			saved_responses.erase(it);
		}
		else
		{
			std::optional<Request>	parsed_request;
			Server					*config = find_active_server(fd, servers);
			RequestParser			parser;
			Route_rule 				*route = nullptr;
			int 					cgi_fd;
			t_cgi					*is_cgi = find_cgi(cgi, fd);
		
			if (is_cgi != nullptr)
			{
				cgi_fd = fd;
				fd = is_cgi->sock;
			}
			try
			{
				if (is_cgi == nullptr)
				{
					parsed_request	= receive_data(fd, parser, servers, keep_alive);
					if (!parsed_request.has_value())
					{
						close_socket(fd, servers, keep_alive);
						continue;
					}
					route = &find_correct_route(config, parsed_request.value());
					if (new_cgi(route->root + "/" + parsed_request->getPath().substr(route->route.length()), config, parsed_request.value(), cgi, fd, envp))
					{
						saved_requests.emplace((std::pair<int, Request>){fd, parsed_request.value()});
						saved_configs.emplace((std::pair<int, Server>){fd, *config});
						saved_routes.emplace((std::pair<int, Route_rule>){fd, *route});
					}
					else
					{
						Response response(config, route, &parsed_request.value(), fd, envp, cookies);
						saved_responses.emplace(fd, response);
						to_respond.push_back(fd);
					}
				}
				else if (is_cgi)
					execute_cgi(fd, saved_requests, saved_configs, saved_routes, cgi, envp, cgi_fd, cookies);
			}
			catch(const HttpParseException& e)
			{
				std::cerr << e.what() << '\n';
				try
				{
					Response error_response(fd, config, e.getStatus(), cookies);
					saved_responses.emplace(fd, error_response);
					to_respond.push_back(fd);
				}
				catch(const std::exception& error)
				{
					std::cerr << "Failed to send error response: " << error.what() << '\n';
					close_socket(fd, servers, keep_alive);
				}
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error handling request: " << e.what() << '\n';
				try
				{
					Response error_response(fd, config, ReplyStatus::InternalServerError, cookies);
					saved_responses.emplace(fd, error_response);
				}
				catch (const std::exception& error)
				{
					std::cerr << "Failed to send error response: " << error.what() << '\n';
					close_socket(fd, servers, keep_alive);
				}
			}
		}

		}
}
