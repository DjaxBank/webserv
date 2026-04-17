#include "Response.hpp"
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sys/socket.h>
#include <filesystem>
#include "cgi.hpp"

Response::Response(const int fd, const Server *config, const Request *request, std::string status) : cgi_fd(-1), config(config),fd(fd), request(request), status(status), Date(get_timestr()) {};

Response::Response(const Server *config, const Request *request, const int fd, char **envp, int cgi_fd)
	: prevcgi(true), cgi_fd(cgi_fd), config(config), envp(envp), fd(fd), request(request), status("200 OK"), method(request->getMethod()), Date(get_timestr()) {};

Response::Response(const Server *config, const Route_rule *route, const Request *request, const int fd, char **envp)
	: cgi_fd(-1), config(config), envp(envp), fd(fd), request(request), route(route), method(request->getMethod()), Date(get_timestr())
{
	if (!route->redirection.empty())
		status = "301 Moved Permanently";
	else
	{
		file_location = route->root + "/" + request->getPath().substr(route->route.length());
		if (this->status.empty())
		{
			std::error_code ec;
			if (std::filesystem::exists(file_location, ec))
			{
				if (ec)
				{
					if (ec.value() == EACCES)
						this->status = "403 Forbidden";
					else
						this->status = "500 Internal Server Error";
				}
				else
				{
					std::ifstream test(file_location);
					if (!test.is_open())
						this->status = "403 Forbidden";
					else
						this->status = "200 OK";
				}
			}
			else
				this->status = "404 Not Found";
		}
	}
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
	if (!content_type.empty())
		return true;
	if (file_location.find_last_of('.') != file_location.npos)
	{
		const std::map<std::string, std::string> types = {
			{".html", "text/html"},
			{".css", "text/css"},
			{".js", "application/javascript"},
			{".png", "image/png"},
			{".jpg", "image/jpeg"},
			{".txt", "text/plain"}};
		std::map<std::string, std::string>::const_iterator it = types.find(file_location.substr(file_location.find_last_of('.')));
		if (it != types.end())
		{
			content_type = it->second;
			return true;
		}
	}
	return false;
}

void Response::ServeDirectory(std::string &path)
{
	body = "<!DOCTYPE html>\n<html lang= \"en\">\n\t<head>\n\t\t<meta charset=\"UTF-8\" />\n\t\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />\n";
	std::string folder;

	if (route->route != "/")
		folder = route->route.substr(1) + "/";
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
		body += "<a href =\"/" + folder + entry + "\"" + "\t<p>" + entry + "</p>\n";
	}
	body += "</html>";
	content_type = "text/html";
}

void Response::ExtractFile(std::string file_path)
{

	if (std::filesystem::is_directory(file_path))
	{
		if (route->directorylisting)
			ServeDirectory(file_path);
		else if (!route->default_dir_file.empty())
			file_path = route->default_dir_file;
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
}

void Response::GET()
{
	ExtractFile(file_location);
	find_contentype();
}

void Response::POST()
{
	const std::map<std::string, std::string> &headers = request->getHeaders();

	if (headers.find("content-type")->second.find("multipart/form-data") == 0)
	{
		if (std::atoi(headers.find("content-length")->second.c_str()) <= config->MaxRequestBodySize)
		{
			std::string uploaded_file(request->getBodyAsString());
			std::string filename(uploaded_file.substr(uploaded_file.find("filename=") + 10));
			filename = filename.substr(0, filename.find('\"'));
			uploaded_file = uploaded_file.substr(uploaded_file.find("\r\n\r\n") + 4);
			uploaded_file = uploaded_file.substr(0, uploaded_file.find("\r\n"));
			std::string serverloc(file_location + "/" + filename);
			std::ofstream file_on_server(serverloc);
			file_on_server << uploaded_file;
			status = "201 Created";
		}
		else
			status = "413 Content Too Large";
	}
}

void Response::DELETE()
{
	std::filesystem::remove(file_location);
}

void Response::SetErrorPages()
{
	if (status == "403 Forbidden" || status == "404 Not Found")
	{
		if (status == "403 Forbidden")
		{
			if (config->Forbidden.empty())
			{
				body = R"HTML(<!DOCTYPE html>
	<html lang="en">
	<head>
		<meta charset="UTF-8" />
		<meta name="viewport" content="width=device-width, initial-scale=1.0" />
		<title>403 Forbidden</title>
	</head>
	<body>
		<main class="card">
			<h1>403</h1>
			<p>Sorry, you can't access this page.</p>
		</main>
	</body>
	</html>)HTML";
			}
			else
				ExtractFile(config->Forbidden);
		}
		else if (status == "404 Not Found")
		{
			if (config->NotFound.empty())
			{
				body = R"HTML(<!DOCTYPE html>
	<html lang="en">
		<head>
			<meta charset="UTF-8" />
			<meta name="viewport" content="width=device-width, initial-scale=1.0" />
			<title>404 Not Found</title>
		</head>
		<body>
			<main class="card">
				<h1>404</h1>
				<p>Sorry, the page you’re looking for doesn’t exist..</p>
			</main>
		</body>
	</html>)HTML";
			}
			else
				ExtractFile(config->NotFound);
		}
	}
	else
	{
		const std::string left = R"HTML(<!DOCTYPE html>
	<html lang="en">
		<head>
			<meta charset="UTF-8" />
			<meta name="viewport" content="width=device-width, initial-scale=1.0" />
			<title>)HTML";
		const std::string right = R"HTML(</title>
	</head>
	</html>)HTML";
		body = left + status + right;
	}
}

void Response::extractcgiheaders()
{
	size_t start_body = body.find("\r\n\r\n");
	if (start_body != body.npos)
	{
		std::string cgiheaders = body.substr(0, start_body);
		body.erase(0, start_body + 4);
		while(!cgiheaders.empty())
		{
			size_t pos = cgiheaders.find("\r\n");
			if (pos == cgiheaders.npos )
				pos = cgiheaders.length();
			std::string cur = cgiheaders.substr(0, pos);
			if (cur.starts_with("Content-type: "))
				content_type = cur.substr(14);
			else
				headers.push_back(cur);
			cgiheaders.erase(0, pos + 2);
		}
	}
}

void Response::Reply()
{
	if (status == "200 OK" || status.empty())
	{
		if (this->prevcgi)
		{
			body = read_cgi(cgi_fd);
			if (!body.empty())
				extractcgiheaders();
			else
				status = "500 Internal Server Error";

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
	}
	else if (status != "301 Moved Permanently")
		SetErrorPages();
	std::string	to_send;
	std::cout << this->config->sock.client_fds.find(fd)->second << ": " << method_tostring(this->request->getMethod()) << ' ' << this->request->getPath() << ' ' << status << ' ' << "(connection " << std::to_string(fd) << ")\n";
	headers.emplace(headers.begin(), "HTTP/1.1 " + status);
	headers.emplace_back("Date: " + Date);
	if (status == "301 Moved Permanently")
		headers.emplace_back("Location: " + route->redirection);
	if (!content_type.empty())
		headers.emplace_back("Content-type: " + content_type);
	headers.emplace_back("content-length: " + std::to_string(body.length()));
	headers.emplace_back("Connection: keep-alive");
 	for (std::string &header : headers)
	{
		to_send += header;
		to_send += "\r\n";
	}
	to_send += "\r\n";
	to_send += body;
	size_t send_bytes = 0;
	while (send_bytes < to_send.length())
	{
		ssize_t val = send(fd, to_send.c_str() + send_bytes, to_send.length() - send_bytes, MSG_NOSIGNAL);
		if (val < 0)
			throw std::runtime_error("error sending to socket");
		send_bytes += val;
	}
}

Response::~Response(){}