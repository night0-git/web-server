#include "server.h"
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <fcntl.h>

int set_flag(int fd, int flag) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | flag);
}

int start_server(struct sockaddr_in *addr) {
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1) {
        perror("socket");
        return -1;
    }

    if (set_flag(listen_sock, O_NONBLOCK) == -1) {
        perror("fcntl");
        return -1;
    }
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1) {
        perror("setsockopt");
        return -1;
    }

    if (bind(listen_sock, (struct sockaddr *)addr, sizeof(*addr)) == -1) {
        perror("bind");
        return -1;
    }

    if (listen(listen_sock, LISTEN_BACKLOG) == -1) {
        perror("listen");
        return -1;
    }

    return listen_sock;
}