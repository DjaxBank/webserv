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
	bool							directorylisting  = false;
	std::string						upload_dir;
};

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
		std::vector<std::pair<int, std::string>>			listen_specs;

		Server(std::ifstream &fstream, char **envp);
		~Server();
};