#pragma once
#include <vector>
#include <sys/select.h>
#include <cstdlib>
#include "Socket.hpp"
#include "requestParser.hpp"
#include "config.hpp"

void handle_client(const Config &config, fd_set *socket_fds, std::vector<Socket> &sockets);
void Http_Get(const int &fd, const Host &Host, const RequestParser &parser);