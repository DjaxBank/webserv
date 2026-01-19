#include "config.hpp"
#include <fstream> 
#include <string>
#include <iostream>

void Config::ImportIntPortPairs(std::ifstream fstream, size_t linecount)
{
	std::string line;
	for (size_t i = 0 ; i < linecount ; i++)
		std::getline(fstream, line);
}

Config::Config(const char *ConfigFile)
{
	std::ifstream fstream(ConfigFile);
	std::string line;
	const std::string config_options[] {
		"Interfaces:",
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
	size_t linecount = 0;
	while (fstream.is_open() && !fstream.eof())
	{
		std::getline(fstream, line);
		linecount++;
		for (size_t i = 0; i < 4; i++)
		{
			size_t pos = line.find(config_options[i]);
			if (pos != std::string::npos)
			{
				std::string value = line.substr(pos + config_options[i].length());
				if (i == 0)
					ImportIntPortPairs(std::ifstream(ConfigFile), linecount);
				else
					*config_locs[i] = value;
			}
		}
	} 
}

Config::Config(const Config &other)
{

}

Config &Config::operator=(const Config &other)
{

}

Config::~Config()
{

}