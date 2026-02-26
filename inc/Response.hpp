#include "Request.hpp"
#include "requestParser.hpp"
#include "Server.hpp"

class Response
{
	private:
		const int			fd;
		const RequestParser	&parser;
		const Route_rule	&route;
		std::string			status;
		HttpMethod			method;
		std::string			Date;
		std::string			Forbiddenpage;
		std::string			NotFoundPage;
		std::string			content_type;
		std::string			content_length;
		std::string			file_location;
		std::string			redirect;
		std::string			body;
		std::string 		ExtractFile(std::string file_path, size_t *total_bytes);
		void 				GET();
		void 				POST();
		void 				DELETE();
		std::string			get_timestr();
		bool				find_contentype();
		void				Send(std::string data);
							Response();
	public:
		Response(const Server &config, const Route_rule &route, const RequestParser &parser, const int fd);
		Response(const Server &config, const Route_rule &route, const RequestParser &parser, const int fd, std::string status);
		void	Reply();
		~Response();
};