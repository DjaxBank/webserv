#pragma once
#include <map>
#include <unistd.h>
#include <string>

std::pair<pid_t, int> start_Cgi(std::string cgi_program, std::string file, char **envp);
std::string read_cgi(int fd);