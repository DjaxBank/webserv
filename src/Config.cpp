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
		missing_str.push_back(" - listen =");
	}
	for (size_t i = 0; i < 3; i++)
	{
		if (str_options[i]->empty())
		{
			has_missing = true;
			missing_str.push_back(str_messages[i]);
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

Config::Config(const char *ConfigFile)
{
	std::ifstream fstream(ConfigFile);
	std::string line;
	const std::string config_options[] {
		"listen =",
		"403 =",
		"404 =",
		"MaxRequestBodySize ="
	};
	std::string *config_locs[]
	{
		nullptr,
		&this->Forbidden,
		&this->NotFound,
		&this->MaxRequestBodySize
	};
	while (fstream.is_open() && !fstream.eof())
	{
		std::getline(fstream, line);
		for (size_t i = 0; i < 4; i++)
		{
			size_t pos = line.find(config_options[i]);
			if (pos != std::string::npos)
			{
				std::string value = line.substr(pos + config_options[i].length());
				size_t start = value.find_first_not_of(' ');
				size_t end = value.find_last_not_of(' ');
				value.erase(end + 1);
				value.erase(0, start);
				if (i == 0)
					ImportIntPortPairs(value);
				else
					*config_locs[i] = value;
				line.clear();
				break ;
			}
		}
		if (!line.empty())
		{
			throw Config::UnknownOptionException();
		}
	}
	CheckAllFull();
}

Config::~Config()
{

}