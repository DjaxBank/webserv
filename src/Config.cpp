#include "config.hpp"
#include <fstream> 
#include <string>
#include <iostream>

void Config::CheckAllFull()
{
	std::string *str_options[]	{&Forbidden, &NotFound, &MaxRequestBodySize};
	const char *str_messages[]	{" - 403 =", " - 404 =", " - MaxRequestBodySize ="};
	std::vector<std::string>	missing_str;
	bool						has_missing = false;
	if (socket_pairs.empty())
	{
		has_missing = true;
		missing_str.emplace_back(" - listen =");
	}
	for (size_t i = 0; i < 3; i++)
	{
		if (str_options[i]->empty())
		{
			has_missing = true;
			missing_str.emplace_back(str_messages[i]);
		}
	}
	if (has_missing == true)
	{
		std::cerr << "Error setting up config, missing directives:";
		for (std::string &current : missing_str)
		{
			std::cerr << '\n' << current;
		}
		std::cerr << '\n';
		throw MissingOptionException();
	}
}

void Config::ImportIntPortPairs(std::string value)
{
	size_t i = 0;
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
			size_t right	= value.find_first_of(' ', delimpos);
			if (right == value.npos)
				right = value.length();
			socket_pair new_pair {value.substr(left, delimpos), std::atoi(value.substr(delimpos + 1, right).c_str())};
			socket_pairs.push_back(new_pair);
			i = delimpos + 1;
		}
	}
}

void Config::ImportRoute(std::string raw, std::ifstream &fstream, size_t linec)
{
	Route_rule new_route;
	std::string line;
	bool first_content = false;
	while(!line.find('}'))
	{
		std::getline(fstream, line);
		if (!line.empty())
		{
			if (first_content == false && !line.find('{'))
				throw std::runtime_error("route at line "+  std::to_string(linec) + " not properly enclosed with { and }");
		}
	}
	routes.push_back(new_route);
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
				size_t start = value.find_first_not_of(' ');
				size_t end = value.find_last_not_of(' ');
				value.erase(end + 1);
				value.erase(0, start);
				if (i == 0)
					ImportIntPortPairs(value);
				else if (i == 1)
					ImportRoute(value, fstream, linec);
				else
					*config_locs[i] = value;
				break ;
			}
		}
		if (!valid_option && !line.empty())
		{
			throw std::runtime_error("unknown directive at line " + std::to_string(linec) + ": " + line);
		}
	}
	CheckAllFull();
}

Config::~Config()
{

}