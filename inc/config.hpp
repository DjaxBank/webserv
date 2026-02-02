#pragma once
#include <vector>
#include <string>
#include <exception>
#include "route.hpp"

struct socket_pair
{
	std::string		interface;
	int				port;
};

class Config
{
	private:
		Config();
		Config(const Config &other);
		Config &operator=(const Config &other);
		void	ImportIntPortPairs(std::string value);
		void	ImportRoute(std::ifstream &fstream, size_t &linec);
		void	CheckAllFull();		
	public:	
		std::vector<socket_pair>	socket_pairs;
		std::string					Forbidden;
		std::string					NotFound;
		std::string					MaxRequestBodySize;
		std::vector<Route_rule>		routes;

		Config(const char *ConfigFile);
		~Config();
		class MissingOptionException : public std::exception
		{
			public:
				const char *what() const throw(){return nullptr;};
		};
};