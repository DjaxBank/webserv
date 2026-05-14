#pragma once
#include <vector>
#include <string>
#include <exception>
#include <map>
#include <optional>
#include <utility>
#include "requestParser.hpp"
#include "Socket.hpp"

struct Route_rule
{
	std::string						route;
	std::string						root;
	std::vector<HttpMethod>			http_methods;
	std::string						redirection;
	std::string						default_dir_file;
	// initialized to false so we dont reject routes that dont have a directorylisting directive
	bool							directorylisting  = false;
	// djax-todo: we need to handle uploads in a separate directory, im storing the dir here
	std::string						upload_dir;
};

// added line c to be able to report lines in errors
class Server
{
	private:
		Server();
		void	ImportPortPairs(const std::string &value, size_t linec);
		void	loadServerConfig(std::ifstream &fstream, size_t &linec, bool &in_server_block);
		void	ImportRoute(std::ifstream &fstream, size_t &linec);
		void	applyServerDirective(const std::string &directive, const std::string &value, size_t linec);
		void	importCgiDirective(const std::string &value, size_t linec);
		void	CheckAllFull();	
		std::string find_cgi_path(std::string cgi_program, char **envp);
	
	public:
		char												**envp;
		std::map<std::string, std::string>					cgiconfigs;
		Socket												sock;
		std::map<int, std::string>							error_page_paths;
		int													MaxRequestBodySize;
		std::vector<Route_rule>								routes;
		// djax-todo: main/handle_client currently we only listen on one port, we need to listen on all ports
		std::vector<std::pair<int, std::string>>			listen_specs;

		Server(std::ifstream &fstream, char **envp);
		~Server();
};