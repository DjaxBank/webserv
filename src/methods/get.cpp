#include "config.hpp"
#include "requestParser.hpp"
#include "Response.hpp"


void	Http_Get(const int fd, const Route_rule &route, const RequestParser &parser)
{
	Response		response(route, parser);


	response.Send(fd);
}