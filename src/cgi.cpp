#include <unistd.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include "Server.hpp"

static char **setenv(char **envp, std::string &query)
{
	if (query.empty())
		return envp;
	else
	{
		query.insert(0, "QUERY_STRING=");
		size_t i = 0;
		while (envp[i] != NULL)
			i++;
		i += 2;
		char **execenv = new char *[i];
		i = 0;
		while (envp[i] != NULL)
		{
			execenv[i] = envp[i];
			i++;
		}
		execenv[i++] = const_cast<char*>(query.c_str());
		execenv[i] = nullptr;
		return execenv;
	}
}

static std::pair<pid_t, int> start_Cgi(std::string cgi_program, std::string file, std::string body, std::string query, int sock, char **envp)
{
	int pipes[2];
	int bodypipe[2];
	char **execenv = setenv(envp, query);

	if (!body.empty())
	{
		pipe(bodypipe);
		write(bodypipe[1], body.c_str(), body.length());
		close(bodypipe[1]);
	}
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
		if (!body.empty())
			dup2(bodypipe[0], STDIN_FILENO);
		execve(cgi_program.c_str(), args_execve, execenv);
		exit(1);
	}
	if (!body.empty())
		close(bodypipe[0]);
	delete[] args_execve;
	if (!query.empty())
		delete [] execenv;
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

bool new_cgi(std::string file_location, Server &config, std::string body, std::string query, std::map<int, int> &cgi, int fd, char **envp)
{
	if (!cgi.contains(fd))
	{
		std::string ext;
		size_t i = file_location.find_last_of('.');
		if (i != file_location.npos)
			ext = file_location.substr(file_location.find_last_of('.'));
		if (config.cgiconfigs.contains(ext))
		{
			cgi.emplace(start_Cgi(config.cgiconfigs.find(ext)->second, file_location, body, query, fd, envp));
			return true;
		}
	}
	return false;
}