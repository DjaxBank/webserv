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

class Server
{
	private:
		Server();
		void	ImportPortPairs(std::string value);
		void	ImportRoute(std::ifstream &fstream, size_t &linec);
		void	CheckAllFull();	
	
	public:	
		CGI							cgi;
		Socket						sock;
		std::string					Forbidden;
		std::string					NotFound;
		unsigned int				MaxRequestBodySize;
		std::vector<Route_rule>		routes;

		Server(std::ifstream &fstream);
		~Server();
};