#include <asm-generic/errno-base.h>
#include <asm-generic/socket.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include "parser.h"

#define BUF_SIZE 4096
#define MAX_EVENTS 10
#define MAX_FDS 65535

enum conn_state {
    CONN_READING,
    CONN_PARSING,
    CONN_WRITING,
    CONN_CLOSING,
};

struct connection {
    int fd;
    struct sockaddr_in addr;

    char buf[BUF_SIZE];
    size_t buf_len;

    enum conn_state state;
};

int set_flag(int fd, int flag) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | flag);
}

size_t find_eoh(char *buf, size_t buf_len) {
    if (buf_len < 4) {
        return SIZE_MAX;
    }
    for (size_t i = 0; i < buf_len - 3; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' &&
            buf[i + 2] == '\r' && buf[i + 3] == '\n') {
            return i + 4;
        }
    }
    return SIZE_MAX;
}

int shift_buf(char *buf, size_t *buf_len, char *data, size_t data_len) {
    if (data_len > *buf_len) {
        return -1;
    }

    memcpy(data, buf, data_len);

    size_t shift = *buf_len - data_len;
    memmove(buf, buf + data_len, shift);
    *buf_len = shift;

    return 0;
}

int main() {
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1) {
        perror("socket");
        return 1;
    }

    if (set_flag(listen_sock, O_NONBLOCK) == -1) {
        perror("fcntl");
        return -1;
    }
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1) {
        perror("setsockopt");
        return -1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr = { INADDR_ANY },
    };

    if (bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(listen_sock, 10) == -1) {
        perror("listen");
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
    };
    struct epoll_event ev = {
        .events = EPOLLIN,
        .data.ptr = &server_conn,
    };
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("epoll_ctl (listen_sock)");
        return 1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    static struct connection clients[MAX_FDS];
    struct epoll_event events[MAX_EVENTS];

    for (;;) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            return 1;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.ptr == &server_conn) {
                client_addr_len = sizeof(client_addr);
                int conn = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);

                if (conn == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue;
                    }
                    perror("accept");
                    return 1;
                }
                if (set_flag(conn, O_NONBLOCK) == -1) {
                    perror("fcntl");
                    return 1;
                }

                // Populate connection struct
                clients[conn].fd = conn;
                clients[conn].addr = client_addr;
                clients[conn].buf_len = 0;

                // Add client socket to epoll instance
                struct epoll_event client_ev = {
                    .events = EPOLLIN,
                    .data.ptr = &clients[conn],
                };
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &client_ev) == -1) {
                    perror("epoll_ctl (client_sock)");
                    return 1;
                }

                // Add the client info to the client list
                printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            } else {
                // Handle client
                struct connection *conn = (struct connection *)events[i].data.ptr;
                ssize_t bytes;
                size_t remain = sizeof(conn->buf) - conn->buf_len;
                if (remain <= 0) {
                    // (?)
                    fprintf(stderr, "Buffer full, dropping data\n");
                    conn->buf_len = 0;
                    continue;
                }
                while ((bytes = read(conn->fd, conn->buf + conn->buf_len, remain)) > 0) {
                    conn->buf_len += bytes;
                    size_t eoh = find_eoh(conn->buf, conn->buf_len);
                    char data[BUF_SIZE];
                    if (eoh != SIZE_MAX) {
                        shift_buf(conn->buf, &conn->buf_len, data, eoh);
                        parse_request(data, eoh);
                    }
                }
                if (bytes == 0) {
                    printf("Client disconnected: %s:%d\n", inet_ntoa(conn->addr.sin_addr), ntohs(conn->addr.sin_port));
                    close (conn->fd);
                } else if (bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("read");
                    return 1;
                }
            }
        }
    }

    return 0;
}