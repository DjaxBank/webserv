#pragma once
#include <vector>
#include <sys/select.h>
#include <cstdlib>
#include "Socket.hpp"

void handle_client(fd_set *socket_fds, std::vector<Socket> &sockets);