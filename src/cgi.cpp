#include <unistd.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <fstream>
#include <map>

std::pair<pid_t, int> start_Cgi(std::string cgi_program, std::string file, char **envp)
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
	return (std::pair<pid_t, int>{pid, pipes[0]});
}

std::string read_cgi(int fd)
{
	std::string php_response;
	char buff[1024];
	while(1)
	{
		ssize_t bytes_read = read(fd, buff, 1024);
		php_response.append(buff, bytes_read);
		if (bytes_read <= 0)
			break ;
	}
	return php_response;
}