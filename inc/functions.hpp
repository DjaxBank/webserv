#pragma once
#include <vector>
#include <sys/select.h>
#include <cstdlib>
#include "Socket.hpp"
#include "requestParser.hpp"
#include "Server.hpp"

void handle_client(std::vector<Server> &config, fd_set *socket_fds);
void Http_Get(const int fd, const Route_rule &route, const RequestParser &parser);