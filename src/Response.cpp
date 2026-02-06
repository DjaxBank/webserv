#include "Response.hpp"
#include "requestParser.hpp"
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sys/socket.h>

Response::Response(const Config &config, const Route_rule &route, const RequestParser &parser, const int fd, std::string status) 
	: fd(fd), parser(parser), status(status), method(parser.getMethod()), Date(get_timestr()), Forbiddenpage(config.Forbidden), NotFoundPage(config.NotFound), file_location(route.root + parser.getTarget()) {}

Response::Response(const Config &config, const Route_rule &route, const RequestParser &parser, const int fd) 
	: fd(fd), parser(parser), method(parser.getMethod()), Date(get_timestr()), Forbiddenpage(config.Forbidden), NotFoundPage(config.NotFound), file_location(route.root + parser.getTarget())
{
	if (access(file_location.c_str(), F_OK) == -1)
		status = "404 Not Found";
	else if (access(file_location.c_str(), R_OK) == -1)
		status = "403 Forbidden";
	else
		status = "200 OK";
}


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


void Response::Send(std::string data)
{
	send(fd, data.c_str(), data.length(), MSG_NOSIGNAL);
}


std::string Response::ExtractFile(std::string file_path, size_t *total_bytes)
{	
	std::string body;
	std::ifstream file(file_path);
	if (total_bytes)
	*total_bytes = 0;
	while (true)
	{
		char buff[1024];
		file.read(buff, sizeof(buff));
		size_t bytes_read = file.gcount();
		if (bytes_read == 0)
		break;
		if (total_bytes)
		*total_bytes += bytes_read;
		body.append(buff, bytes_read);
	}
	return body;
}

void Response::GET()
{
	if (find_contentype())
	this->Send("Content-Type: " + content_type + "\r\n");
	size_t total_bytes;
	body = ExtractFile(file_location, &total_bytes);
	this->Send("content-length: " + std::to_string(total_bytes) + "\r\n");
}

void Response::POST()
{
	
}

void Response::DELETE()
{
	
}


void Response::Reply()
{
	std::cout << status << '\n';
	this->Send("HTTP/1.1 " + status + "\r\n");
	this->Send("Date: " + Date + "\r\n");
	this->Send("Connection: close\r\n");
	if (status != "200 OK")
	{
		if (status == "403 Forbidden")
			body = ExtractFile(Forbiddenpage, nullptr);
		else if (status == "404 Not Found")
			body = ExtractFile(NotFoundPage, nullptr);
	}
	else
	{
		switch (method)
		{
			case (HttpMethod::GET):
				this->GET();
				break;
			case (HttpMethod::POST):
				this->POST();
				break;
			case (HttpMethod::DELETE):
				this->DELETE();
				break;
			default:
				break;
		}
	}
	this->Send("\r\n");
	if (!body.empty())
		this->Send(body);
}

Response::~Response()
{
	
}
