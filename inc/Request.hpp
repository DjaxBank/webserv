#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

class Request
{
	private:
		std::string m_method;
		std::string m_target;
		std::string m_version;
		std::map<std::string, std::string> m_headers;
		std::string m_body;
	public:
		Request();
		Request(const Request& other);
		Request& operator=(const Request& other);
		~Request();

		const std::string& getMethod() const;
		const std::string& getTarget() const;
		const std::string& getVersion() const;
		const std::map<std::string, std::string>& getHeaders() const;
		const std::string& getBody() const;

		void setMethod(const std::string& method);
		void setTarget(const std::string& target);
		void setVersion(const std::string& version);
		void setHeaders(const std::map<std::string, std::string>& headers);
		void setBody(const std::string& body);
};

#endif