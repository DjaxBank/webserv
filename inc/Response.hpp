#include "Request.hpp"
#include "route.hpp"
#include "requestParser.hpp"

class Response
{
	private:
		const int		fd;
		std::string		status;
		HttpMethod		method;
		std::string		content_type;
		std::string		content_length;
		std::string		Date;
		std::string		file_location;
		Response();
		std::string		get_timestr();
		bool			find_contentype();
		void			Send(std::string data);
	public:
		Response(const Route_rule &route, const RequestParser &parser, const int fd, HttpMethod method);
		void	Reply();
};