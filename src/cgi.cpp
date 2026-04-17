#include <unistd.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <map>
#include "Server.hpp"
#include "requestParser.hpp"

static char **setenv(char **envp, std::vector<std::pair<std::string, std::string> *> &to_add, std::vector<std::string> &final_strings)
{
	size_t		i = 0;
	
	for (std::pair<std::string, std::string> *cur : to_add)
	{
		if (!cur->second.empty())
		final_strings.push_back(cur->first + '=' + cur->second);
	}
	final_strings.push_back("REDIRECT_STATUS=200");
	while (envp[i] != NULL)
		i++;
	char **execenv = new char *[i + final_strings.size() + 1];
	i = 0;
	while (envp[i] != NULL)
	{
		execenv[i] = envp[i];
		i++;
	}
	for (std::string &cur : final_strings)
	{
			execenv[i] = const_cast<char *>(cur.c_str());
			i++;
	}
	execenv[i] = nullptr;
	return execenv;
}

static std::pair<pid_t, int> start_Cgi(Server &config, std::string cgi_program, std::string scriptname, std::string filelocation, Request &request, int sock, char **envp)
{
	const std::map<std::string, std::string >headers = request.getHeaders();
	std::pair<std::string, std::string>	REQUEST_METHOD("REQUEST_METHOD", method_tostring(request.getMethod()));
	std::pair<std::string, std::string>	QUERY_STRING("QUERY_STRING", request.getQuery());
	std::pair<std::string, std::string>	CONTENT_TYPE("CONTENT_TYPE", headers.contains("content-type") ? headers.find("content-type")->second : "");
	std::pair<std::string, std::string>	CONTENT_LENGTH("CONTENT_LENGTH", headers.contains("content-length") ? headers.find("content-length")->second : ""); 
	std::pair<std::string, std::string>	SCRIPT_NAME("SCRIPT_NAME", scriptname);
	std::pair<std::string, std::string>	SCRIPT_FILENAME("SCRIPT_FILENAME", filelocation);
	std::pair<std::string, std::string>	SERVER_NAME("SERVER_NAME", headers.contains("host") ? headers.find("host")->second : "");
	std::pair<std::string, std::string>	SERVER_PORT("SERVER_PORT", std::to_string(config.sock.info.first));
	std::pair<std::string, std::string>	SERVER_PROTOCOL("SERVER_PROTOCOL", "HTTP/1.1");
	std::pair<std::string, std::string>	REMOTE_ADDR("REMOTE_ADDR", config.sock.client_fds.find(sock)->second);
	std::vector<std::pair<std::string, std::string> *> to_add{&REQUEST_METHOD, &QUERY_STRING, &CONTENT_TYPE, &CONTENT_LENGTH, &SCRIPT_NAME, &SCRIPT_FILENAME, &SERVER_NAME, &SERVER_PORT, &SERVER_PROTOCOL, &REMOTE_ADDR};
	std::vector<std::string> final_strings;
	std::string body = request.getBodyAsString();
	int pipes[2];
	int bodypipe[2];
	char **execenv = setenv(envp, to_add, final_strings);

	if (!request.getBodyAsString().empty())
	{
		pipe(bodypipe);
		write(bodypipe[1], body.c_str(), body.length());
		close(bodypipe[1]);
	}
	pipe(pipes);
	std::vector<std::string>	args;
	args.push_back(cgi_program);
	args.push_back(filelocation);
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
			cgi.emplace(start_Cgi(config, config.cgiconfigs.find(ext)->second, request.getPath(), file_location, request, fd, envp));
			return true;
		}
	}
	return false;
}