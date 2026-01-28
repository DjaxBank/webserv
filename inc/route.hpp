#pragma once
#include <string>
#include <vector>
#include "Request.hpp"

struct Route_rule
{
		std::vector<HttpMethod>			http_methods;
		std::string						route;
		std::string						redirection;
		std::string						root;
		std::string						default_dir_file;
		bool							directoryListing;
};