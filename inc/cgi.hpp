#pragma once
#include <map>
#include <unistd.h>
#include <string>

std::string read_cgi(int fd);
bool new_cgi(std::string file_location, Server &config, std::string body, std::string query, std::map<int, int> &cgi, int fd, char **envp);