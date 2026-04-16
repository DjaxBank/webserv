#pragma once
#include <map>
#include <unistd.h>
#include <string>

std::string read_cgi(int fd);
bool new_cgi(std::string file_location, Server &config, Request &request, std::map<int, int> &cgi, int fd, char **envp);