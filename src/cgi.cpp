#include <unistd.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <fstream>

std::string Cgi(std::string cgi_program, std::string file, char **envp)
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

	int status;
	waitpid(pid, &status, 0);
	std::string php_response;
	if (status == EXIT_SUCCESS)
	{
		char buff[1024];
		size_t bytes_read;
		while(1)
		{
			bytes_read = read(pipes[0], buff, 1024);
			php_response.append(buff, bytes_read);
			if (bytes_read <= 0)
				break ;
		}
	}
	close (pipes[0]);
	return php_response;
}