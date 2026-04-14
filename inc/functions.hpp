#pragma once
#include <vector>
#include <sys/select.h>
#include <cstdlib>
#include "Socket.hpp"
#include "requestParser.hpp"
#include "Server.hpp"

void	handle_client(std::vector<Server> &servers, fd_set *socket_fds, std::vector<int> &keep_alive, std::map<pid_t, int> &cgi, char **envp);
void	Http_Get(const int fd, const Route_rule &route, const RequestParser &parser);
void	close_socket(int fd, std::vector<Server> &servers, std::vector<int> &keep_alive);