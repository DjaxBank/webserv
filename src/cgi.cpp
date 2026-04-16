#include <unistd.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <map>
#include "Server.hpp"
#include "requestParser.hpp"

static char **setenv(char **envp, Request &request, Server &config, int sock)
{
	size_t		i = 0;
	std::string	REQUEST_METHOD(method_tostring(request.getMethod()));
	std::string QUERY_STRING(request.getQuery());
	std::string CONTENT_TYPE;
	std::string SCRIPT_NAME;
	std::string PATH_INFO;
	std::string SERVER_NAME;
	std::string SERVER_PORT;
	std::string SERVER_PROTOCOL;
	std::string REMOTE_ADDR(config.sock.client_fds.find(sock)->second);
	size_t		paramcount = 8 + !QUERY_STRING.empty();

	while (envp[i] != NULL)
		i++;
	const size_t totalcount = i + paramcount + 1;
	if (!QUERY_STRING.empty())
		QUERY_STRING.insert(0, "QUERY_STRING=");
	char **execenv = new char *[totalcount];
	while (envp[i] != NULL)
	{
		execenv[i] = envp[i];
		i++;
	}
	std::vector<std::string *> to_add{&REQUEST_METHOD, &QUERY_STRING, &CONTENT_TYPE, &SCRIPT_NAME, &PATH_INFO, &SERVER_NAME, &SERVER_PORT, &SERVER_PROTOCOL, &REMOTE_ADDR};
	for (std::string *cur : to_add)
	{
		if (!cur->empty())
		{
			execenv[i] = const_cast<char *>(cur->c_str());
			i++;
		}
	}
	execenv[i] = nullptr;
	return execenv;
}

static std::pair<pid_t, int> start_Cgi(Server &config, std::string cgi_program, std::string file, Request &request, int sock, char **envp)
{
	int pipes[2];
	int bodypipe[2];
	char **execenv = setenv(envp, request, config, sock);
	std::string body = request.getBodyAsString();

	if (!request.getBodyAsString().empty())
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
	if (!request.getQuery().empty())
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

bool new_cgi(std::string file_location, Server &config, Request &request, std::map<int, int> &cgi, int fd, char **envp)
{
	if (!cgi.contains(fd))
	{
		std::string ext;
		size_t i = file_location.find_last_of('.');
		if (i != file_location.npos)
			ext = file_location.substr(file_location.find_last_of('.'));
		if (config.cgiconfigs.contains(ext))
		{
			cgi.emplace(start_Cgi(config, config.cgiconfigs.find(ext)->second, file_location, request, fd, envp));
			return true;
		}
	}
	return false;
}