#include "Request.hpp"
#include "Server.hpp"

class Response
{
	private:
		bool				prevcgi = false;
		const int			cgi_fd;
		const				Server *config;
		char				**envp;
		const int			fd;
		const Request		*request;
		const Route_rule	*route;
		std::string			status;
		HttpMethod			method;
		std::string			Date;
		std::string			content_type;
		std::string			file_location;
		std::vector<std::string>	headers;
		std::string			body;
		void 				ServeDirectory(std::string &path);
		void				ExtractFile(std::string file_path);
		void 				GET();
		void 				POST();
		void 				DELETE();
		std::string			get_timestr();
		bool				find_contentype();
							Response();
		bool				MethodAllowed();
		void				SetErrorPages();
		void				extractcgiheaders();
		bool				is_cgi();
	public:
		Response(const int fd, std::string status); // error constructor
		Response(const Server *config, const Request *request, const int fd, char **envp, int cgi_fd); //cgi constructor
		Response(const Server *config, const Route_rule *route, const Request *request, const int fd, char **envp); //normal constructor
		void	Reply();
		~Response();
};