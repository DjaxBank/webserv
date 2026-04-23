#include "Response.hpp"
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sys/socket.h>
#include <filesystem>
std::string Cgi(std::string cgi_program, std::string file, char **envp);

// if we are on an error path, we need to set things null to prevent crashes
Response::Response(const int fd, ReplyStatus status)
	: config(NULL), envp(NULL), fd(fd), request(NULL), route(NULL), status(status), method(HttpMethod::NONE), Date(get_timestr()) {};

Response::Response(const Server *config, const Route_rule *route, const Request *request, const int fd, char **envp)
	: config(config), envp(envp), fd(fd), request(request), route(route), method(request->getMethod()), Date(get_timestr())
{
	this->status = ReplyStatus::Unset;
	if (!route->redirection.empty())
		this->status = ReplyStatus::MovedPermanently;
	else
	{
		file_location = route->root + "/" + request->getPath().substr(route->route.length());
		std::error_code ec;
		if (std::filesystem::exists(file_location, ec))
		{
			if (ec)
			{
				if (ec.value() == EACCES)
					this->status = ReplyStatus::Forbidden;
				else
					this->status = ReplyStatus::InternalServerError;
			}
			else
			{
				std::ifstream test(file_location);
				if (!test.is_open())
					this->status = ReplyStatus::Forbidden;
				else
					this->status = ReplyStatus::OK;
			}
		}
		else
			this->status = ReplyStatus::NotFound;
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
		auto it = types.find(file_location.substr(file_location.find_last_of('.')));
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

bool Response::is_cgi(std::string &p_cgi)
{
	size_t i = file_location.find_last_of('.');
	std::string ext;
	if (i != file_location.npos)
		ext = file_location.substr(file_location.find_last_of('.'));
	if (config->cgiconfigs.contains(ext))
	{
		p_cgi = config->cgiconfigs.find(ext)->second;
		return true;
	}
	return false;
}

void Response::GET()
{
	std::string p_cgi;

	if (is_cgi(p_cgi))
	{
		body = Cgi(p_cgi, file_location, envp);
		if (body.empty())
			this->status = ReplyStatus::InternalServerError;
		size_t headers_end = body.find("\r\n\r\n");
		if (headers_end != body.npos)
		{
			std::string	cgi_headers = body.substr(0, headers_end + 2);
			body.erase(0, headers_end + 4);
			while (!cgi_headers.empty())
			{
				size_t delimloc = cgi_headers.find("\r\n");
				std::string new_header = cgi_headers.substr(0, delimloc);
				if (new_header.find("Content-type:") == 0)
					content_type = new_header.substr(14);
				cgi_headers.erase(0, delimloc + 2);
			}
		}
	}
	else
	{
		ExtractFile(file_location);
		find_contentype();
	}
}


// added != .end() guards to check if the headers are actually present and protect against crashes
void Response::POST()
{
	const std::map<std::string, std::string> &headers = request->getHeaders();
	std::map<std::string, std::string>::const_iterator content_type_it = headers.find("content-type");
	std::map<std::string, std::string>::const_iterator content_length_it = headers.find("content-length");

	if (content_type_it != headers.end() && content_type_it->second.find("multipart/form-data") == 0
		&& content_length_it != headers.end())
	{
		if (std::atoi(content_length_it->second.c_str()) <= config->MaxRequestBodySize)
		{
			std::string uploaded_file(request->getBodyAsString());
			std::string filename(uploaded_file.substr(uploaded_file.find("filename=") + 10));
			filename = filename.substr(0, filename.find('\"'));
			uploaded_file = uploaded_file.substr(uploaded_file.find("\r\n\r\n") + 4);
			uploaded_file = uploaded_file.substr(0, uploaded_file.find("\r\n"));
			std::string serverloc(file_location + "/" + filename);
			std::ofstream file_on_server(serverloc);
			file_on_server << uploaded_file;
			status = ReplyStatus::Created;
		}
		else
			status = ReplyStatus::ContentTooLarge;
	}
}

void Response::DELETE()
{
	std::filesystem::remove(file_location);
}

bool Response::MethodAllowed()
{
	bool allowed = false;
	for (HttpMethod cur : route->http_methods)
		if (cur == method)
		{
			allowed = true;
			break;
		}
	return allowed;
}

static std::string status_to_string(ReplyStatus status)
{
	switch(status)
	{
		case ReplyStatus::OK: return "200 OK";
		case ReplyStatus::Created: return "201 Created";
		case ReplyStatus::MovedPermanently: return "301 Moved Permanently";
		case ReplyStatus::BadRequest: return "400 Bad Request";
		case ReplyStatus::Forbidden: return "403 Forbidden";
		case ReplyStatus::NotFound: return "404 Not Found";
		case ReplyStatus::MethodNotAllowed: return "405 Method Not Allowed";
		case ReplyStatus::RequestTimeout: return "408 Request Timeout";
		case ReplyStatus::ContentTooLarge: return "413 Content Too Large";
		case ReplyStatus::UriTooLong: return "414 URI Too Long";
		case ReplyStatus::RequestHeaderFieldsTooLarge: return "431 Request Header Fields Too Large";
		case ReplyStatus::NotImplemented: return "501 Not Implemented";
		case ReplyStatus::InternalServerError:
		default: return "500 Internal Server Error";
	}
}

// todo -> add reply status to string mapper here
// added content_type to error pages (if it is empty, set it to text/html)
// maybe we should add a default content type to the response class instead of setting it in the error pages?
// config can be null in one response path -> so in both cases check if it is null and if so, use the default error pages
// at the end if body is empty, set it to the default error page and set the content type to text/html (fallthrough case)
void Response::SetErrorPages()
{
	if (status == ReplyStatus::Forbidden)
	{
		if (config && !config->Forbidden.empty())
		{
			ExtractFile(config->Forbidden);
			if (content_type.empty())
				content_type = "text/html";
		}
		else
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
			content_type = "text/html";
		}
	}
	else if (status == ReplyStatus::NotFound)
	{
		if (config && !config->NotFound.empty())
		{
			ExtractFile(config->NotFound);
			if (content_type.empty())
				content_type = "text/html";
		}
		else
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
			content_type = "text/html";
		}
	}
	if (body.empty())
	{
		content_type = "text/html";
		body = "<!DOCTYPE html><html><body><h1>" + status_to_string(status) + "</h1></body></html>";
	}
}

// guard unset status and set to internal server error
// guard method not allowed and set to method not allowed to not overwrite more specific errors
// method checks only happen on healthy path, otherwise it will fall through to errors or to SetErrorPages()
// added headers.clear in case of old headers being present from previous responses
// added guards to check if request and route are not null to prevent crashes
// send returns ssize_t (signed int) so we need to cast the result to size_t to prevent overflows
void Response::Reply()
{
	std::string	to_send;

	if (status == ReplyStatus::Unset)
		status = ReplyStatus::InternalServerError;
	if (request && route)
	{
		if (!MethodAllowed() && status == ReplyStatus::OK)
			status = ReplyStatus::MethodNotAllowed;
		else if (status == ReplyStatus::OK)
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
					status = ReplyStatus::NotImplemented;
					break;
			}
		}
	}
	if (status != ReplyStatus::OK && status != ReplyStatus::Created && status != ReplyStatus::MovedPermanently)
		SetErrorPages();

	std::cout << "status: " << status_to_string(status) << std::endl;
	headers.clear();
	headers.emplace_back("HTTP/1.1 " + status_to_string(status));
	headers.emplace_back("Date: " + Date);
	if (status == ReplyStatus::MovedPermanently && route && !route->redirection.empty())
		headers.emplace_back("Location: " + route->redirection);
	if (!content_type.empty())
		headers.emplace_back("Content-type: " + content_type);
	headers.emplace_back("Content-length: " + std::to_string(body.length()));
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
		send_bytes += static_cast<size_t>(val);
	}
}

Response::~Response(){}