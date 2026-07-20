#include "connection.h"
#include "server.h"
#include "event_loop.h"
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <stdbool.h>

int main() {
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr = { INADDR_ANY },
    };
    int listen_sock = start_server(&server_addr);
    if (listen_sock == -1) {
        perror("start_server");
        return 1;
    }

    // Create the epoll instance
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
        return 1;
    }

    // Add the listen socket
    struct connection server_conn = {
        .fd = listen_sock,
        .buf_len = 0,
        .addr = server_addr,
    };
    if (add_conn(epfd, EPOLLIN, &server_conn, listen_sock) == -1) {
        perror("add_conn");
        return 1;
    }

    if (start_event_loop(epfd, &server_conn) == -1) {
        perror("start_event_loop");
        return 1;
    }

    return 0;
}