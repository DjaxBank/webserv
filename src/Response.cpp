#include "Response.hpp"
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sys/socket.h>
#include <filesystem>

Response::Response(const Server &config, const Route_rule &route, const Request &request, const int fd, std::string status) 
	: fd(fd), request(request), route(route),  status(status), method(request.getMethod()), Date(get_timestr()), Forbiddenpage(config.Forbidden), NotFoundPage(config.NotFound), file_location(route.root + request.getRawUri()), redirect(route.redirection) {}

Response::Response(const Server &config, const Route_rule &route, const Request &request, const int fd) 
	: fd(fd), request(request), route(route), method(request.getMethod()), Date(get_timestr()), Forbiddenpage(config.Forbidden), NotFoundPage(config.NotFound), file_location(route.root + request.getRawUri())
{
	std::error_code ec;
	if (std::filesystem::exists(file_location, ec))
	{
		if (ec)
		{
			if (ec.value() == EACCES)
				status = "403 Forbidden";
			else
				status = "500 Internal Server Error";
		}
		else
		{
			std::ifstream test(file_location);
			if (!test.is_open())
				status = "403 Forbidden";
			else
				status = "200 OK";
		}
	}
	else
		status = "404 Not Found";
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

void Response::ServeDirectory(std::string &path)
{
	body = "<!DOCTYPE html>\n<html lang= \"en\">\n\t<head>\n\t\t<meta charset=\"UTF-8\" />\n\t\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n";

	for (auto &file : std::filesystem::directory_iterator(path))
	{
		std::string entry = file.path();
		if (entry == path + "index.html")
		{
			path = file.path();
			body.clear();
			return;
		}
		entry = entry.substr(path.length());
		body += "<a href =\"/" + entry + "\"" + "\t<p>" + entry + "</p>\n";
	}
	body += "</html>";
	content_type = "text/html";
}

void Response::ExtractFile(std::string file_path)
{

	if (std::filesystem::is_directory(file_path))
	{
		if (route.directorylisting)
			ServeDirectory(file_path);
		else if (!route.default_dir_file.empty())
			file_path = route.default_dir_file;
	}
	if (body.empty())
	{
		std::ifstream file(file_path);
		while (true)
		{
			char buff[1024];
			file.read(buff, sizeof(buff));
			size_t bytes_read = file.gcount();
			if (bytes_read == 0)
				break;
			body.append(buff, bytes_read);
		}
	}
		total_bytes = body.length();
}

void Response::GET()
{
	ExtractFile(file_location);
	find_contentype();
}

void Response::POST()
{
	const std::map<std::string, std::string>& headers = request.getHeaders();
	std::string check;
	std::map<std::string, std::string>::const_iterator it = headers.find("content-type");
	if (it != headers.end())
		check = it->second;
	if (check.find("multipart/form-data") == 0)
	{
		auto upload_raw = request.getBody();
		std::string uploaded_file;
		status = "201 Created";
	}
}

void Response::DELETE()
{
	
}

void Response::Reply()
{
	if (status != "200 OK")
	{
		if (status == "403 Forbidden")
		ExtractFile(Forbiddenpage);
		else if (status == "404 Not Found")
		ExtractFile(NotFoundPage);
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
	std::cout << status << '\n';
	this->Send("HTTP/1.1 " + status + "\r\n");
	this->Send("Date: " + Date + "\r\n");
	if (status == "301 Moved permanently")
		this->Send("Location: " + redirect + "\r\n");
	if(!content_type.empty())
		this->Send("Content-Type: " + content_type + "\r\n");
	if (total_bytes > 0)
		this->Send("content-length: " + std::to_string(total_bytes) + "\r\n");
	this->Send("Connection: close\r\n");
	this->Send("\r\n");
	if (!body.empty())
		this->Send(body);
}

Response::~Response()
{
	
}
