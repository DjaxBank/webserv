#pragma once
#include "Request.hpp"
#include "Server.hpp"
#include <map>
#include <unistd.h>
#include <string>
#include <chrono>

typedef struct t_cgi
{
	int 	pipe;
	int		sock;
	pid_t	pid;
	std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
	bool active = true;
} t_cgi;

std::string read_cgi(int fd);
bool new_cgi(std::string file_location, Server &config, Request &request, std::vector<t_cgi> &cgi, int fd, char **envp);
t_cgi *find_cgi(std::vector<t_cgi> &cgi, const int to_find);
void check_timeout(std::vector<t_cgi> &cgi);