#pragma once
#include <vector>
#include <string>
#include <exception>
#include <map>
#include "requestParser.hpp"
#include "Socket.hpp"

struct CGI
{

		// TO IMPLEMENT

};


struct Route_rule
{
	std::string						route;
	std::string						root;
	std::vector<HttpMethod>			http_methods;
	std::string						redirection;
	std::string						default_dir_file;
	bool							directorylisting;
};

class Config
{
	private:
	Config();
	Config(const Config &other);
	Config &operator=(const Config &other);
	std::map<int, std::string> 	ImportPortPairs(std::string value);
	void	ImportRoute(std::ifstream &fstream, size_t &linec);
	void	CheckAllFull();
	std::vector<Socket> setup_sockets(const std::map<int, std::string> &pairs);		
	
	public:	
		CGI							cgi;
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