#include "Server.hpp"
#include "functions.hpp"
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#include <unordered_set>
#include <sstream>
#include <cctype>

// finds the first and last non-whitespace characters and returns the substring between them
static std::string trim(const std::string &str)
{
	const char *whitespace = " \t\n\r\f\v";
	size_t start = str.find_first_not_of(whitespace);
	size_t end = str.find_last_not_of(whitespace);
	if (start == std::string::npos || end == std::string::npos)
		return "";
	return str.substr(start, end - start + 1);
}

// reads one line, increments linec, returns full-line trimmed content (may be empty)
static std::string readTrimmedLine(std::ifstream &fstream, size_t &linec)
{
	std::string line;
	std::getline(fstream, line);
	linec++;
	return trim(line);
}

enum LineTokenKind
{
	Empty,
	OpenBrace,
	CloseBrace,
	Directive,
	Invalid
};

struct LineToken
{
	LineTokenKind kind;
	std::string directive;
	std::string value;
};

// Tokenizes a full line that has already been trimmed (see readTrimmedLine). Trims again
// around '=' for directive key/value in case of internal spaces.
LineToken tokenizeLine(const std::string &trimmed)
{
	LineToken token;
	token.kind = Empty;
	token.directive = "";
	token.value = "";

	if (trimmed.empty())
		return token;
	if (trimmed == "{")
		token.kind = OpenBrace;
	else if (trimmed == "}")
		token.kind = CloseBrace;
	else if (trimmed.find('=') != std::string::npos)
	{
		size_t equal_pos = trimmed.find('=');
		token.kind = Directive;
		token.directive = trim(trimmed.substr(0, equal_pos));
		token.value = trim(trimmed.substr(equal_pos + 1));
	}
	else
		token.kind = Invalid;
	return token;
}

static bool validateHttpStatus(const std::string &directive)
{
	if (directive.size() != 3)
		return false;
	for (unsigned char ch : directive)
	{
		if (!std::isdigit(ch))
			return false;
	}
	const int code = std::atoi(directive.c_str());
	return code >= 100 && code <= 599;
}

// checks for valid named server directives and HTTP status codes
static bool isAllowedServerDirective(const std::string &directive)
{
	static const std::unordered_set<std::string> named = {
		"listen",
		"MaxRequestBodySize",
		"cgi"
	};

	if (named.count(directive))
		return true;
	else if (validateHttpStatus(directive))
		return true;
	return false;
}

std::string Server::find_cgi_path(std::string cgi_program, char **envp)
{
	std::string					path;
	std::vector<std::string>	folders;

	for (size_t i = 0; envp[i] != NULL; i++)
	{
		path = envp[i];
		if (path.find("PATH=") == 0)
			break;
	}
	if (path.find("PATH=") != 0)
		return "";
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

void Server::ImportPortPairs(const std::string &value, size_t linec)
{
	size_t delimpos = value.find_first_of(':', 0);
	if (delimpos == std::string::npos)
		throw std::runtime_error("missing ':' in listen directive at line " + std::to_string(linec) + ": " + value);
	size_t left 	= value.find_last_of(' ', delimpos);
	if (left == value.npos)
		left = 0;
	else
		left++;
	size_t right	= value.find_first_of(' ', delimpos) - 1;
	if (right == value.npos)
		right = value.length();
	const int port = std::atoi(value.substr(delimpos + 1, right - delimpos).c_str());
	const std::string host = value.substr(left, delimpos - left);
	listen_specs.push_back({port, host});
}

// validates the required fields for a route rule
static void validateRequiredFields(Route_rule &new_route)
{
	std::vector<std::string> missing_options;
	if (new_route.route.empty())
		missing_options.push_back("route");
	if (new_route.root.empty())
		missing_options.push_back("root");
	if (new_route.default_dir_file.empty())
		missing_options.push_back("default");

	if (!missing_options.empty())
	{
		std::string error_message = "Error setting up route, missing directives:";
		for (std::string &current : missing_options)
			error_message += '\n' + current;
		error_message += '\n';
		throw std::runtime_error(error_message);
	}
}

// whitespace-separated tokens only -> get, post, delete, one required
static void importRouteMethods(const std::string &value, Route_rule &new_route, size_t linec)
{
	bool have_get = false, have_post = false, have_delete = false;
	std::istringstream iss(value);
	std::string word;
	while (iss >> word)
	{
		std::string lower;
		lower.reserve(word.size());
		for (unsigned char ch : word)
			lower += static_cast<char>(std::tolower(ch));
		if (lower == "get")
			have_get = true;
		else if (lower == "post")
			have_post = true;
		else if (lower == "delete")
			have_delete = true;
		else
			throw std::runtime_error("invalid method in methods directive at line " + std::to_string(linec) + ": " + word);
	}
	if (!have_get && !have_post && !have_delete)
		throw std::runtime_error("methods directive needs at least one of get, post, delete at line " + std::to_string(linec));
	if (have_get)
		new_route.http_methods.push_back(HttpMethod::GET);
	if (have_post)
		new_route.http_methods.push_back(HttpMethod::POST);
	if (have_delete)
		new_route.http_methods.push_back(HttpMethod::DELETE);
}

// takes a directive and a route rule and dispatches the directive to the appropriate field in the route rule
static void dispatchDirective(LineToken &token, Route_rule &new_route, size_t &linec)
{
	if (token.directive == "route")
		new_route.route = token.value;
	else if (token.directive == "root")
		new_route.root = token.value;
	else if (token.directive == "default")
		new_route.default_dir_file = token.value;
	else if (token.directive == "directorylisting")
		new_route.directorylisting = (token.value == "true");
	else if (token.directive == "redirection")
		new_route.redirection = token.value;
	else if (token.directive == "methods")
		importRouteMethods(token.value, new_route, linec);
	else if (token.directive == "upload")
		new_route.upload_dir = token.value;
	else
		throw std::runtime_error("unknown directive at line " + std::to_string(linec) + ": " + token.directive);
}

// reads a route block and validates the route rule and adds it to the routes vector
void Server::ImportRoute(std::ifstream &fstream, size_t &linec)
{
	Route_rule new_route;
	int depth = 0;
	while (fstream.is_open() && !fstream.eof())
	{
		const std::string line = readTrimmedLine(fstream, linec);
		if (line.empty())
			continue;
		LineToken token = tokenizeLine(line);
		if (token.kind == OpenBrace)
			depth++;
		else if (token.kind == CloseBrace)
			depth--;
		else if (depth == 0 && token.kind == Directive)
			throw std::runtime_error("unexpected directive at line " + std::to_string(linec) + ": " + token.directive);
		if (depth < 0)
			throw std::runtime_error("unexpected closing brace at line " + std::to_string(linec));
		else if (depth > 0 && token.kind == Directive)
			dispatchDirective(token, new_route, linec);
		if (depth == 0 && token.kind == CloseBrace)
			break;
	}
	if (depth != 0)
    	throw std::runtime_error("unclosed route block starting near line " + std::to_string(linec));
	validateRequiredFields(new_route);
	routes.push_back(new_route);
}

void Server::applyServerDirective(const std::string &directive, const std::string &value, size_t linec)
{
	if (!isAllowedServerDirective(directive))
		throw std::runtime_error("unknown directive at line " + std::to_string(linec) + ": " + directive);
	if (directive == "listen")
		ImportPortPairs(value, linec);
	else if (directive == "MaxRequestBodySize")
		MaxRequestBodySize = std::atoi(value.c_str());
	else if (directive == "cgi")
		importCgiDirective(value, linec);
	else
		error_page_paths[std::atoi(directive.c_str())] = value;
}

// helper function to import the cgi directive
void Server::importCgiDirective(const std::string &value, size_t linec)
{
	size_t space = value.find(' ');
	if (space == value.npos)
		throw std::runtime_error("malformed cgi directive at line " + std::to_string(linec) + ": " + value);
	std::string extension = value.substr(0, space);
	std::string program = value.substr(space + 1);
	std::string cgi_path = find_cgi_path(program, envp);
	if (cgi_path.empty())
		throw std::runtime_error(program + " does not exist");
	cgiconfigs.emplace(extension, cgi_path);
}

void Server::loadServerConfig(std::ifstream &fstream, size_t &linec, bool &in_server_block)
{
	while (fstream.is_open() && !fstream.eof())
	{
		const std::string line = readTrimmedLine(fstream, linec);
		if (line.empty())
			continue;
		if (line == "route")
		{
			if (!in_server_block)
				throw std::runtime_error("route block outside server at line " + std::to_string(linec));
			ImportRoute(fstream, linec);
			continue;
		}

		LineToken token = tokenizeLine(line);
		if (token.kind == OpenBrace)
		{
			if (!in_server_block)
				in_server_block = true;
			else
				throw std::runtime_error("unexpected opening brace at line " + std::to_string(linec));
		}
		else if (token.kind == CloseBrace)
		{
			if (in_server_block)
			{
				in_server_block = false;
				break;
			}
			else
				throw std::runtime_error("unexpected closing brace at line " + std::to_string(linec));
		}
		else if (token.kind == Directive)
		{
			if (!in_server_block)
				throw std::runtime_error("directive outside server block at line " + std::to_string(linec));
			applyServerDirective(token.directive, token.value, linec);
		}
		else if (token.kind == Invalid)
			throw std::runtime_error("invalid token at line " + std::to_string(linec));
	}
	if (in_server_block)
		throw std::runtime_error("unclosed server block near line " + std::to_string(linec));
}

Server::Server(std::ifstream &fstream, char **envp) : envp(envp)
{
	size_t linec = 0;
	bool in_server_block = false;

	loadServerConfig(fstream, linec, in_server_block);
	// djax-todo: i think the subject says we need to listen on all ports, not just the first one
	if (!listen_specs.empty())
		sock = Socket(listen_specs.front());
}

Server::~Server()
{

}