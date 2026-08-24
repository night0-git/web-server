#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

#define LISTEN_BACKLOG 4096

int set_flag(int fd, int flag);

int start_server(struct sockaddr_in *addr);

#endif