#pragma once
#include <map>
#include <unistd.h>
#include <string>

std::pair<int, int> start_Cgi(std::string cgi_program, std::string file, std::string body, int sock, char **envp);
std::string read_cgi(int fd);
bool new_cgi(std::string file_location, Server &config, std::string body, std::map<int, int> &cgi, int fd, char **envp);