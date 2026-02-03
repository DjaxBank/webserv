#include "Response.hpp"
#include "requestParser.hpp"
#include "route.hpp"
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sys/socket.h>

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

Response::Response(const Config &config, const Route_rule &route, const RequestParser &parser, const int fd, HttpMethod method) 
	: fd(fd), method(method), Date(get_timestr()), file_location(route.root + parser.getTarget())
{
	Forbiddenpage = config.Forbidden;
	NotFoundPage = config.NotFound;
	if (status.empty())
	{
		if (access(file_location.c_str(), F_OK) == -1)
			status = "404 Not Found";
		else if (access(file_location.c_str(), R_OK) == -1)
			status = "403 Forbidden";
		else
			status = "200 OK";
	}
}

void Response::Send(std::string data)
{
	if (send(fd, data.c_str(), data.length(), MSG_NOSIGNAL) < 0)
		std::cerr << "client disconnected\n";
}


std::string Response::ExtractFile(std::string file_path, size_t *total_bytes)
{	
	std::string body;
	std::ifstream file(file_path);
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
		Send("Content-Type: " + content_type + "\r\n");
	size_t total_bytes = 0;
	body = ExtractFile(file_location, &total_bytes);
	Send("content-length: " + std::to_string(total_bytes) + "\r\n");
}

void Response::POST()
{

}

void Response::DELETE()
{

}


void Response::Reply()
{
	Send("HTTP/1.1 " + status + "\r\n");
	Send("Date: " + Date + "\r\n");
	Send("Connection: close\r\n");
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
	Send("\r\n");
	Send(body);
}

Response::~Response()
{
	
}
