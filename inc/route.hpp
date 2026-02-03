#pragma once
#include <string>
#include <vector>
#include "Request.hpp"

struct Route_rule
{
	std::string						route;
	std::string						root;
	std::vector<HttpMethod>			http_methods;
	std::string						redirection;
	std::string						default_dir_file;
	bool							directorylisting;
};