#pragma once
#include <string>
#include <vector>

struct UrlRule
{
		std::vector<std::string>		http_methods;
		std::string						redirection;
		std::string						directory;
		bool							directoryListing;
};