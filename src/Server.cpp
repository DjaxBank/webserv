#include "Server.hpp"
#include "functions.hpp"
#include <fstream> 
#include <string>
#include <iostream>
#include <filesystem>

std::string Server::find_cgi_path(std::string cgi_program, char **envp)
{
	std::string					path;
	std::vector<std::string>	folders;

	for (size_t i = 0 ; envp[i] != NULL ; i++)
	{
		path = envp[i];
		if (path.find("PATH=") == 0)
			break;
	}
	path.erase(0, path.find_first_of('=') + 1);
	while (!path.empty())
	{
		size_t split_loc = path.find_first_of(':');
		if (split_loc == path.npos)
			split_loc = path.length();
		folders.push_back(path.substr(0, split_loc));
		path.erase(0, split_loc + 1);
	}
	for (std::string &cur : folders)
	{
		std::string test = cur + "/" + cgi_program;
		if (std::filesystem::exists(test))
			return test;
	}
	return "";
}

void Server::ImportPortPairs(std::string value)
{
	size_t delimpos = value.find_first_of(':', 0);
	size_t left 	= value.find_last_of(' ', delimpos);
	if (left == value.npos)
		left = 0;
	else
		left++;
	size_t right	= value.find_first_of(' ', delimpos) - 1;
	if (right == value.npos)
		right = value.length();
	sock = Socket({std::atoi(value.substr(delimpos + 1, right - delimpos).c_str()), value.substr(left, delimpos - left)});
}

void Server::ImportRoute(std::ifstream &fstream, size_t &linec)
{
	const size_t route_start = linec;
	Route_rule new_route;
	std::vector<std::string>	route_raw;
	bool open, closed, dir_list_present = false;
	while(fstream.is_open()&& !closed)
	{
		std::string line;
		std::getline(fstream, line);
		linec++;
		if (!line.empty())
		{
			if (line.find('{') != line.npos)
			{
				open = true;
				line.erase(0, line.find('{') + 1);
			}
			if (!line.empty())
			route_raw.push_back(line);
		}
		if (line.find('}') != line.npos)
			closed = true;
	}
	if (!open || !closed)
		throw std::runtime_error("route at line " +  std::to_string(route_start) + " not properly enclosed with { and }");
	for (std::string &str : route_raw)
	{
		if (str.find("route =") != str.npos)
			new_route.route = str.substr(str.find("route =") + 8);
		else if (str.find("root = ") != str.npos)
			new_route.root = str.substr(str.find("root =") + 7);
		else if (str.find("default = ") != str.npos)
			new_route.default_dir_file = str.substr(str.find("default =") + 10);
		else if (str.find("directorylisting =") != str.npos)
		{
			dir_list_present = true;
			new_route.directorylisting = str.substr(str.find("directorylisting =") + 19) == "true";
		}
		else if (str.find("redirection = ") != str.npos)
			new_route.redirection = str.substr(str.find("redirection =") + 14);
		else if (str.find("methods = ") != str.npos)
		{
			if (str.find("get") != str.npos)
				new_route.http_methods.push_back(HttpMethod::GET);
			if (str.find("post") != str.npos)
				new_route.http_methods.push_back(HttpMethod::POST);
			if (str.find("delete") != str.npos)
				new_route.http_methods.push_back(HttpMethod::DELETE);
		}
	}
	std::vector<std::string> missing_options;
	const std::string	route_options[] {
		"route",
		"root",
		"default",
		"methods"
	};
	std::string			*route_locs[] {
		&new_route.route,
		&new_route.root,
		&new_route.default_dir_file,
	};
	for (size_t i = 0; i < 3; i++)
		if (route_locs[i]->empty())
			missing_options.push_back(route_options[i]);
	if (dir_list_present == false)
		missing_options.push_back("directorylisting");
	routes.push_back(new_route);
	if (!missing_options.empty())
	{
		std::cerr << "Error setting up route, missing directives:";
		for (std::string &current : missing_options)
			std::cerr << '\n' << current;
		std::cerr << '\n';
		throw std::runtime_error("");
	}
}

Server::Server(std::ifstream &fstream, char **envp) : envp(envp)
{
	size_t				linec = 0;
	bool				open, closed = false;
	const std::string	server_options[] {
		"listen =",
		"route",
		"403 =",
		"404 =",
		"MaxRequestBodySize =",
		"cgi ="
	};

	while (fstream.is_open() && !fstream.eof() && !closed)
	{
		std::string line;
		bool 		valid_option = false;
		std::getline(fstream, line);
		linec++;
		if (line.find('{') != line.npos)
			open = true;
		for (size_t i = 0; i < 6 && !line.empty(); i++)
		{
			size_t pos = line.find(server_options[i]);
			if (pos != std::string::npos)
			{
				if (!open)
					throw std::runtime_error("unexpected directive at line " + std::to_string(linec) + ": " + line);
				valid_option = true;
				std::string value = line.substr(pos + server_options[i].length());
				value.erase(value.find_last_not_of(' ') + 1);
				value.erase(0, value.find_first_not_of(' '));
				std::string cgi_path;
				switch (i)
				{
					case 0:
						ImportPortPairs(value);
						break;
					case 1:
						ImportRoute(fstream, linec);
						break;
					case 2:
						this->Forbidden = value;
						break;
					case 3:
						this->NotFound = value;
						break;
					case 4:
						MaxRequestBodySize = std::atoi(value.c_str());
						break;
					case 5:
						cgi_path = find_cgi_path(value.substr(value.find(' ') + 1), this->envp);
						if (cgi_path.empty())
							throw std::runtime_error(value.substr(value.find(' ') + 1) + " does not exist");
						cgiconfigs.emplace(value.substr(0, value.find(' ')), cgi_path);
						break;
					default:
						break;
				}
			}
		}
		if (line.find('}') != line.npos)
			closed = true;
		if (!valid_option && !line.empty() && (line.find('{') == line.npos && line.find('}') == line.npos))
			throw std::runtime_error("unknown directive at line " + std::to_string(linec) + ": " + line);
	}
}

Server::~Server()
{

}