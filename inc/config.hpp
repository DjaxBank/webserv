#pragma once
#include <vector>
#include <string>
#include "UrlRule.hpp"

class Config
{
	private:
		Config();
		std::string				Forbidden;
		std::string				NotFound;
		unsigned int			MaxRequestBodySize;
		std::vector<UrlRule>	Url_rules;

	public:	
		Config(const std::string ConfigFile);
		Config(const Config &other);
		Config &operator=(const Config &other);
		~Config();
};