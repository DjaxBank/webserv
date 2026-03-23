#include "Request.hpp"
#include "Server.hpp"

class Response
{
	private:
		const int			fd;
		const Request		*request;
		const Route_rule	*route;
		std::string			status;
		HttpMethod			method;
		std::string			Date;
		std::string			Forbiddenpage;
		std::string			NotFoundPage;
		std::string			content_type;
		std::string			file_location;
		std::string			redirect;
		std::string			body;
		void 				ServeDirectory(std::string &path);
		void				ExtractFile(std::string file_path);
		void 				GET();
		void 				POST();
		void 				DELETE();
		std::string			get_timestr();
		bool				find_contentype();
		void				Send(std::string data);
							Response();
		bool				MethodAllowed();
		void				SetErrorPages();
	public:
		Response(const int fd, std::string status);
		Response(const Server *config, const Route_rule *route, const Request *request, const int fd, std::string status);
		void	Reply();
		~Response();
};