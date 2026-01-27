#pragma once
#include <string>
#include <vector>
#include "Request.hpp"

struct Host
{
		std::vector<HttpMethod>			http_methods;
		std::string						servername;
		std::string						redirection;
		std::string						root;
		std::string						default_dir_file;
		bool							directoryListing;
};