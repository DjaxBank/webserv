#include "Request.hpp"
#include "route.hpp"
#include "requestParser.hpp"

class Response
{
	private:
		std::string		status;
		std::string		content_type;
		std::string		content_length;
		std::string		Date;
		std::string		file_location;
		Response();
		std::string		get_timestr();
		bool			find_contentype();
	public:
		Response(const Route_rule &route, const RequestParser &parser);
		void	Send(const int fd);
};