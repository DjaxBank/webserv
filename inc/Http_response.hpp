#include "Request.hpp"

class Http_response
{
	private:
		unsigned int	code;
		std::string		content_type;
		std::string		content_length;
		std::string		Date;
	public:
		void set_code(unsigned int new_code) {code = new_code;};
};