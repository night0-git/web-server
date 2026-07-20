#include "event_loop.h"
#include "connection.h"
#include "server.h"
#include "parser.h"
#include <stdio.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define MAX_EVENTS 10
#define MAX_FDS 65535

int add_conn(int epfd, uint32_t events, void *data, int fd) {
    struct epoll_event ev = {
        .events = events,
        .data = { data },
    };
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl");
        return -1;
    }
    return 0;
}

int start_event_loop(int epfd, struct connection *server_conn) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    static struct connection clients[MAX_FDS];
    struct epoll_event events[MAX_EVENTS];

    for (;;) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            return -1;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.ptr == server_conn) {
                client_addr_len = sizeof(client_addr);
                int conn = accept(server_conn->fd, (struct sockaddr *)&client_addr, &client_addr_len);

                if (conn == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue;
                    }
                    perror("accept");
                    return -1;
                }
                if (set_flag(conn, O_NONBLOCK) == -1) {
                    perror("fcntl");
                    return -1;
                }

                // Populate connection struct
                clients[conn].fd = conn;
                clients[conn].addr = client_addr;
                clients[conn].buf_len = 0;

                // Add client socket to epoll instance
                if (add_conn(epfd, EPOLLIN, &clients[conn], conn) == -1) {
                    return -1;
                }
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
                    const char *eoh;
                    if ((eoh = strstr(conn->buf, "\r\n\r\n"))) {
                        char data[BUF_SIZE];
                        shift_buf(conn->buf, &conn->buf_len, data, eoh - conn->buf);
                        struct request req;
                        if (parse_request(data, &req) != -1) {
                            printf("Request: %s %s %s\n", req.method, req.path, req.version);
                        } else {
                            printf("Failed to parse request: %.*s\n", (int)(eoh - conn->buf), data);
                        }
                    }
                }
                if (bytes == 0) {
                    printf("Client disconnected: %s:%d\n", inet_ntoa(conn->addr.sin_addr), ntohs(conn->addr.sin_port));
                    close (conn->fd);
                } else if (bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("read");
                    return -1;
                }
            }
        }
    }

    return 0;
}