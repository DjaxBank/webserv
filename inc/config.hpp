#pragma once
#include <vector>
#include <string>
#include "UrlRule.hpp"

class Config
{
	private:
		Config();
		void ImportIntPortPairs(std::ifstream fstream, size_t linecount);
		std::vector<std::string[2]> socket_pairs;
		std::string					Forbidden;
		std::string					NotFound;
		std::string					MaxRequestBodySize;
		std::vector<UrlRule>		Url_rules;

	public:	
		Config(const char *ConfigFile);
		Config(const Config &other);
		Config &operator=(const Config &other);
		~Config();
};