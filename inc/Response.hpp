#include "Request.hpp"
#include "route.hpp"
#include "requestParser.hpp"
#include "config.hpp"

class Response
{
	private:
		const int			fd;
		const RequestParser	&parser;
		std::string			status;
		std::string			Forbiddenpage;
		std::string			NotFoundPage;
		HttpMethod			method;
		std::string			content_type;
		std::string			content_length;
		std::string			Date;
		std::string			file_location;
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
		Response(const Config &config, const Route_rule &route, const RequestParser &parser, const int fd);
		void	Reply();
		~Response();
};