#include "Response.hpp"
#include "requestParser.hpp"
#include "route.hpp"
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <iostream>

std::string Response::get_timestr()
{
	char buff[1024];
	std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	
	std::strftime(buff, sizeof(buff), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&time));
	return buff;
}

void	Response::find_contentype()
{
	if (file_location.find_last_of('.') != file_location.npos)
	{
		const std::map<std::string, std::string> types = {
			{".html", "text/html"},
			{".css", "text/css"},
			{".js", "application/javascript"},
			{".png", "image/png"},
			{".jpg", "image/jpeg"},
			{".txt", "text/plain"}
		};
		auto it = types.find(file_location.substr(file_location.find_last_of('.')));
		if (it != types.end())
			content_type = it->second;
	}
}

Response::Response(const Route_rule &route, const RequestParser &parser) : Date(get_timestr()), file_location(route.root + parser.getTarget())
{
	if (access(file_location.c_str(), F_OK) == -1)
		status = "404 Not Found";
	else if (access(file_location.c_str(), R_OK) == -1)
		status = "403 Forbidden";
	else
		status = "200 OK";
}

void Response::Send(const int fd)
{
	std::string Response_header;

	Response_header = "HTTP/1.1 " + status + "\r\n";
	write(fd, Response_header.c_str(), Response_header.length());
	std::cout << Response_header;
	Response_header = "Date: " + Date + "\r\n";
	write(fd, Response_header.c_str(), Response_header.length());
	std::cout << Response_header;
	find_contentype();
	if (!content_type.empty())
	{
		Response_header = "Content-Type: " + content_type + "\r\n";
		write(fd, Response_header.c_str(), Response_header.length());
		std::cout << Response_header;
	}
	Response_header = "\r\n";
	write(fd, Response_header.c_str(), Response_header.length());
	std::cout << Response_header;
	if (status == "200 OK")
	{
		std::ifstream file(file_location);
		char buff[1024];
		file.read(buff, sizeof(buff));
		while (write(fd, buff, file.gcount()) > 0)
			file.read(buff, sizeof(buff));
	}
}
