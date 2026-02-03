#pragma once
#include <vector>
#include <string>
#include <exception>
#include "route.hpp"
#include "Socket.hpp"

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
	std::vector<socket_pair> 	ImportIntPortPairs(std::string value);
	void	ImportRoute(std::ifstream &fstream, size_t &linec);
	void	CheckAllFull();
	std::vector<Socket> setup_sockets(const std::vector<socket_pair> &pairs);		
	public:	
		std::vector<Socket>			sockets;
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