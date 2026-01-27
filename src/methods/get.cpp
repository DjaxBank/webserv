#include "config.hpp"
#include "requestParser.hpp"
#include "Http_response.hpp"
#include <unistd.h>

void	Http_Get(const int &fd, const Host &Host, const RequestParser &parser)
{
	Http_response	response;
	std::string		file_location = Host.default_dir_file + parser.getTarget();

	if (access(file_location.c_str(), F_OK) == -1)
		response.set_code(404);
	else if (access(file_location.c_str(), R_OK) == -1)
		response.set_code(403);
	else
		response.set_code(200);
}