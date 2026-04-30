#include "Server.hpp"
#include "functions.hpp"
#include <fstream> 
#include <string>
#include <iostream>
#include <filesystem>

// CONFIG PARSER OVERVIEW
// - This file contains the hand-written parser for server blocks and route blocks.
// - Flow:
//   1. `importconfigfile()` in src/main.cpp finds top-level `server` blocks.
//   2. `Server::Server()` consumes a server block and dispatches directives.
//   3. `Server::ImportRoute()` consumes nested route blocks and fills `Route_rule`.
//   4. `ImportPortPairs()` and `find_cgi_path()` are directive-level helpers.
// - Keep these invariants in mind when reading or refactoring:
//   * Block parsing must be brace-aware and fail on missing `{` or `}`.
//   * Values extracted from config lines should be trimmed before use.
//   * Directive matching is currently substring-based and therefore brittle.
//   * Failure cases should include line numbers when possible.

// find_cgi_path(cgi_program, envp)
// - Input: a CGI program name and the process environment.
// - Output: the first PATH entry where the program exists, or "" if not found.
// - Flow: read PATH from envp -> split on ':' -> test each candidate with filesystem::exists.
// - Failure cases to remember:
//   * PATH missing from envp -> returns empty string.
//   * Program not found -> returns empty string.
//   * Existence is checked, but executability is not validated here.
std::string Server::find_cgi_path(std::string cgi_program, char **envp)
{
	std::string					path;
	std::vector<std::string>	folders;

	for (size_t i = 0 ; envp[i] != NULL ; i++)
	{
		path = envp[i];
		if (path.find("PATH=") == 0)
			break;
	}
	path.erase(0, path.find_first_of('=') + 1);
	while (!path.empty())
	{
		size_t split_loc = path.find_first_of(':');
		if (split_loc == path.npos)
			split_loc = path.length();
		folders.push_back(path.substr(0, split_loc));
		path.erase(0, split_loc + 1);
	}
	for (std::string &cur : folders)
	{
		std::string test = cur + "/" + cgi_program;
		if (std::filesystem::exists(test))
			return test;
	}
	return "";
}

// ImportPortPairs(value)
// - Input: the text following `listen =`.
// - Output: sets `sock` from a parsed host:port pair.
// - Flow: locate ':' -> trim spaces around host and port -> convert port -> construct Socket.
// - Failure cases to test/document:
//   * missing ':'
//   * non-numeric port text
//   * extra tokens after the port
//   * leading/trailing spaces around the pair
void Server::ImportPortPairs(std::string value)
{
	size_t delimpos = value.find_first_of(':', 0);
	size_t left 	= value.find_last_of(' ', delimpos);
	if (left == value.npos)
		left = 0;
	else
		left++;
	size_t right	= value.find_first_of(' ', delimpos) - 1;
	if (right == value.npos)
		right = value.length();
	sock = Socket({std::atoi(value.substr(delimpos + 1, right - delimpos).c_str()), value.substr(left, delimpos - left)});
}

// ImportRoute(fstream, linec)
// - Input: the open config stream positioned at a route block and the current line counter.
// - Output: appends a parsed Route_rule to `routes`.
// - Flow:
//   1. Read until a matching `}` is found.
//   2. Collect non-empty block lines into `route_raw`.
//   3. Parse each collected line by substring-matching known directives.
//   4. Validate required route fields and then push the route.
// - Failure cases to document/test:
//   * missing opening `{` or closing `}`
//   * EOF before block close
//   * case-sensitive directive mismatches like `directoryListing`
//   * malformed `methods =` content
//   * missing required fields
void Server::ImportRoute(std::ifstream &fstream, size_t &linec)
{
	const size_t route_start = linec;
	Route_rule new_route;
	std::vector<std::string>	route_raw;
	bool open = false;
	bool closed = false;
	bool dir_list_present = false;
	while(fstream.is_open()&& !closed)
	{
		std::string line;
		std::getline(fstream, line);
		linec++;
		if (!line.empty())
		{
			if (line.find('{') != line.npos)
			{
				open = true;
				line.erase(0, line.find('{') + 1);
			}
			if (!line.empty())
			route_raw.push_back(line);
		}
		if (line.find('}') != line.npos)
			closed = true;
	}
	if (!open || !closed)
		throw std::runtime_error("route at line " +  std::to_string(route_start) + " not properly enclosed with { and }");
	for (std::string &str : route_raw)
	{
		if (str.find("route =") != str.npos)
			new_route.route = str.substr(str.find("route =") + 8);
		else if (str.find("root = ") != str.npos)
			new_route.root = str.substr(str.find("root =") + 7);
		else if (str.find("default = ") != str.npos)
			new_route.default_dir_file = str.substr(str.find("default =") + 10);
		else if (str.find("directorylisting =") != str.npos)
		{
			dir_list_present = true;
			new_route.directorylisting = str.substr(str.find("directorylisting =") + 19) == "true";
		}
		else if (str.find("redirection = ") != str.npos)
			new_route.redirection = str.substr(str.find("redirection =") + 14);
		else if (str.find("methods = ") != str.npos)
		{
			if (str.find("get") != str.npos)
				new_route.http_methods.push_back(HttpMethod::GET);
			if (str.find("post") != str.npos)
				new_route.http_methods.push_back(HttpMethod::POST);
			if (str.find("delete") != str.npos)
				new_route.http_methods.push_back(HttpMethod::DELETE);
		}
	}
	std::vector<std::string> missing_options;
	const std::string	route_options[] {
		"route",
		"root",
		"default",
		"methods"
	};
	std::string			*route_locs[] {
		&new_route.route,
		&new_route.root,
		&new_route.default_dir_file,
	};
	for (size_t i = 0; i < 3; i++)
		if (route_locs[i]->empty())
			missing_options.push_back(route_options[i]);
	if (dir_list_present == false)
		missing_options.push_back("directorylisting");
	routes.push_back(new_route);
	if (!missing_options.empty())
	{
		std::cerr << "Error setting up route, missing directives:";
		for (std::string &current : missing_options)
			std::cerr << '\n' << current;
		std::cerr << '\n';
		throw std::runtime_error("");
	}
}

// Server::Server(fstream, envp)
// - Input: the config stream and process environment.
// - Output: a configured Server instance with socket, routes, CGI mappings, and error pages.
// - Flow:
//   1. Read lines until the server block closes.
//   2. Require directives to appear only after an opening `{`.
//   3. Dispatch recognized directives to specialized helpers.
//   4. Record an error for unknown top-level directives.
// - Failure cases to document/test:
//   * directive before `{`
//   * missing closing `}`
//   * malformed `cgi =` lines
//   * invalid listen syntax or numeric values
//   * unknown directives at the top level
Server::Server(std::ifstream &fstream, char **envp) : envp(envp)
{
	size_t				linec = 0;
	bool				open, closed = false;
	const std::string	server_options[] {
		"listen =",
		"route",
		"403 =",
		"404 =",
		"MaxRequestBodySize =",
		"cgi ="
	};

	while (fstream.is_open() && !fstream.eof() && !closed)
	{
		std::string line;
		bool 		valid_option = false;
		std::getline(fstream, line);
		linec++;
		if (line.find('{') != line.npos)
			open = true;
		for (size_t i = 0; i < 6 && !line.empty(); i++)
		{
			size_t pos = line.find(server_options[i]);
			if (pos != std::string::npos)
			{
				if (!open)
					throw std::runtime_error("unexpected directive at line " + std::to_string(linec) + ": " + line);
				valid_option = true;
				std::string value = line.substr(pos + server_options[i].length());
				value.erase(value.find_last_not_of(' ') + 1);
				value.erase(0, value.find_first_not_of(' '));
				std::string cgi_path;
				switch (i)
				{
					case 0:
						ImportPortPairs(value);
						break;
					case 1:
						ImportRoute(fstream, linec);
						break;
					case 2:
						this->Forbidden = value;
						break;
					case 3:
						this->NotFound = value;
						break;
					case 4:
						MaxRequestBodySize = std::atoi(value.c_str());
						break;
					case 5:
						cgi_path = find_cgi_path(value.substr(value.find(' ') + 1), this->envp);
						if (cgi_path.empty())
							throw std::runtime_error(value.substr(value.find(' ') + 1) + " does not exist");
						cgiconfigs.emplace(value.substr(0, value.find(' ')), cgi_path);
						break;
					default:
						break;
				}
			}
		}
		if (line.find('}') != line.npos)
			closed = true;
		if (!valid_option && !line.empty() && (line.find('{') == line.npos && line.find('}') == line.npos))
			throw std::runtime_error("unknown directive at line " + std::to_string(linec) + ": " + line);
	}
}

Server::~Server()
{

}