#include "config.hpp"
#include <fstream> 
#include <string>
#include <iostream>

void Config::CheckAllFull()
{
	std::string *str_options[]	{&Forbidden, &NotFound, &MaxRequestBodySize};
	const char *str_messages[]	{" - 403 =", " - 404 =", " - MaxRequestBodySize ="};
	std::vector<std::string>	missing_str;

	if (sockets.empty())
		missing_str.emplace_back(" - listen =");
	for (size_t i = 0; i < 3; i++)
	{
		if (str_options[i]->empty())
			missing_str.emplace_back(str_messages[i]);
	}
	if (!missing_str.empty())
	{
		std::cerr << "Error setting up config, missing directives:";
		for (std::string &current : missing_str)
			std::cerr << '\n' << current;
		std::cerr << '\n';
		throw MissingOptionException();
	}
}

std::map<int, std::string>  Config::ImportPortPairs(std::string value)
{
	size_t i = 0;
	std::map<int, std::string> socket_pairs;
	while (true)
	{
		size_t delimpos = value.find_first_of(':', i);
		if (delimpos == value.npos)
			break;
		else
		{
			size_t left 	= value.find_last_of(' ', delimpos);
			if (left == value.npos)
				left = 0;
			else
				left++;
			size_t right	= value.find_first_of(' ', delimpos) - 1;
			if (right == value.npos)
				right = value.length();
			int port = std::atoi(value.substr(delimpos + 1, right - delimpos).c_str());
			std::string interface = value.substr(left, delimpos - left);
			if (socket_pairs.find(port) != socket_pairs.end())
				throw std::runtime_error("double usage of port " + std::to_string(port));
			socket_pairs.emplace(port, interface);
			i = delimpos + 1;
		}
	}
	return socket_pairs;
}

void Config::ImportRoute(std::ifstream &fstream, size_t &linec)
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
	}
	std::vector<std::string> missing_options;
	const std::string	route_options[] {
		"route",
		"root",
		"default"
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
		throw MissingOptionException();
	}
}

std::vector<Socket> Config::setup_sockets(const std::map<int, std::string> &pairs)
{
	std::vector<Socket> sockets;

	sockets.reserve(pairs.size());
	for (std::pair<int, std::string> pair : pairs)
		sockets.emplace_back(pair);
	return sockets;
}

Config::Config(const char *ConfigFile)
{
	std::ifstream		fstream(ConfigFile);
	size_t				linec = 0;
	const std::string	config_options[] {
		"listen =",
		"route",
		"403 =",
		"404 =",
		"MaxRequestBodySize ="
	};
	std::string			*config_locs[] {
		nullptr,
		nullptr, 
		&this->Forbidden,
		&this->NotFound,
		&this->MaxRequestBodySize
	};
	while (fstream.is_open() && !fstream.eof())
	{
		std::string line;
		bool 		valid_option = false;
		std::getline(fstream, line);
		linec++;
		for (size_t i = 0; i < 5 && !line.empty(); i++)
		{
			size_t pos = line.find(config_options[i]);
			if (pos != std::string::npos)
			{
				valid_option = true;
				std::string value = line.substr(pos + config_options[i].length());
				value.erase(value.find_last_not_of(' ') + 1);
				value.erase(0, value.find_first_not_of(' '));
				if (i == 0)
					sockets = setup_sockets(ImportPortPairs(value));
				else if (i == 1)
					ImportRoute(fstream, linec);
				else
					*config_locs[i] = value;
				break ;
			}
		}
		if (!valid_option && !line.empty())
			throw std::runtime_error("unknown directive at line " + std::to_string(linec) + ": " + line);
	}
	CheckAllFull();
}

Config::~Config()
{

}