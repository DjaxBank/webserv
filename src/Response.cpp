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

bool Response::find_contentype()
{
	if (file_location.find_last_of('.') != file_location.npos)
	{
		const std::map<std::string, std::string> types = {
			{".html", "text/html"},
			{".css", "text/css"},
			{".js", "application/javascript"},
			{".png", "image/png"},
			{".jpg", "image/jpeg"},
			{".txt", "text/plain"}};
		auto it = types.find(file_location.substr(file_location.find_last_of('.')));
		if (it != types.end())
		{
			content_type = it->second;
			return true;
		}
	}
	return false;
}

Response::Response(const Route_rule &route, const RequestParser &parser, const int fd, HttpMethod method) 
	: fd(fd), method(method), Date(get_timestr()), file_location(route.root + parser.getTarget())
{
	if (access(file_location.c_str(), F_OK) == -1)
		status = "404 Not Found";
	else if (access(file_location.c_str(), R_OK) == -1)
		status = "403 Forbidden";
	else
		status = "200 OK";
}

void Response::Send(std::string data)
{
	write(fd, data.c_str(), data.length());
}

void Response::Reply()
{
	Send("HTTP/1.0 " + status + "\r\n");
	Send("Date: " + Date + "\r\n");
	std::string body;
	if (status == "200 OK")
	{
		if (find_contentype())
			Send("Content-Type: " + content_type + "\r\n");
		size_t total_bytes = 0;
		std::ifstream file(file_location);
		while (true)
		{
			char buff[1024];
			file.read(buff, sizeof(buff));
			size_t bytes_read = file.gcount();
			if (bytes_read == 0)
				break;
			total_bytes += bytes_read;
			body.append(buff, bytes_read);
		}
		Send("content-length: " + std::to_string(total_bytes) + "\r\n");
	}
	Send("\r\n");
	Send(body);
}
