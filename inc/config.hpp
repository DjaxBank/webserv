#pragma once
#include <vector>
#include <string>
#include <exception>
#include "UrlRule.hpp"

struct socket_pair
{
	std::string		interface;
	int				port;
};

class Config
{
	private:
		Config();
		void						ImportIntPortPairs(std::string value);
		void	CheckAllFull();
		std::vector<socket_pair>	socket_pairs;
		std::string					Forbidden;
		std::string					NotFound;
		std::string					MaxRequestBodySize;
		std::vector<UrlRule>		Url_rules;

	public:	
		Config(const char *ConfigFile);
		Config(const Config &other);
		Config &operator=(const Config &other);
		~Config();
		class UnknownOptionException : public std::exception
		{
			public:
				const char *what() const throw(){return "unknown directive at line ";};
		};
		class MissingOptionException : public std::exception
		{
			public:
				const char *what() const throw(){return nullptr;};
		};
};