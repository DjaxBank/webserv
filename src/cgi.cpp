#include <unistd.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include "Server.hpp"

std::pair<pid_t, int> start_Cgi(std::string cgi_program, std::string file, int sock, char **envp)
{
	int pipes[2];
	pipe(pipes);
	std::vector<std::string>	args;

	args.push_back(cgi_program);
	args.push_back(file);
	size_t size = args.size();
	char **args_execve = new char *[size + 1];
	for (size_t i = 0; i < size; i++)
		args_execve[i] = const_cast <char*>(args[i].c_str());
	args_execve[size] = nullptr;
	pid_t pid = fork();
	if (pid == 0)
	{
		dup2(pipes[1], STDOUT_FILENO);
		execve(cgi_program.c_str(), args_execve, envp);
		exit(1);
	}
	delete[] args_execve;
	close (pipes[1]);
	return (std::pair<int, int>(pipes[0], sock));
}

std::string read_cgi(int fd)
{
	std::string	cgi_response;
	char		buff[1024];
	while(1)
	{
		ssize_t bytes_read = read(fd, buff, 1024);
		if (bytes_read <= 0)
			break ;
		cgi_response.append(buff, bytes_read);
	}
	close(fd);
	return cgi_response;
}

bool new_cgi(std::string file_location, Server &config, std::map<int, int> &cgi, int fd, char **envp)
{
	if (!cgi.contains(fd))
	{
		std::string ext;
		size_t i = file_location.find_last_of('.');
		if (i != file_location.npos)
			ext = file_location.substr(file_location.find_last_of('.'));
		if (config.cgiconfigs.contains(ext))
		{
			cgi.emplace(start_Cgi(config.cgiconfigs.find(ext)->second, file_location, fd, envp));
			return true;
		}
	}
	return false;
}