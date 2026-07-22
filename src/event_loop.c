#include "event_loop.h"
#include "connection.h"
#include "server.h"
#include "parser.h"
#include "response.h"
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

volatile sig_atomic_t sigterm_received = 0;

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
    int num_clients = 0;

    while (!sigterm_received || num_clients > 0) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) {
                continue;
            }
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
                clients[conn].read_len = 0;
                clients[conn].write_len = 0;
                clients[conn].write_offset = 0;
                clients[conn].state = CONN_READING;

                // Add client socket to epoll instance
                if (add_conn(epfd, EPOLLIN, &clients[conn], conn) == -1) {
                    perror("add_conn");
                    return -1;
                }
                num_clients++;
                printf("Client connected: %s:%d (%d)\n",
                       inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port), num_clients);
            } else {
                // Handle client
                struct connection *conn = (struct connection *)events[i].data.ptr;

                ssize_t bytes;
                size_t remain = sizeof(conn->read_buf) - conn->read_len;
                if (remain <= 0) {
                    // (?)
                    fprintf(stderr, "Buffer full, dropping data\n");
                    conn->read_len = 0;
                    continue;
                }

                if (conn->state == CONN_READING) {
                    while ((bytes = read(conn->fd, conn->read_buf + conn->read_len, remain)) > 0) {
                        conn->state = CONN_READING;
                        conn->read_len += bytes;

                        const char *eoh;
                        const char *needle = "\r\n";
                        size_t needle_size = strlen(needle);
                        if ((eoh = memmem(conn->read_buf, conn->read_len, "\r\n", needle_size))) {

                            eoh += needle_size;

                            conn->state = CONN_PARSING;

                            char data[BUF_SIZE];
                            size_t data_len = eoh - conn->read_buf;
                            shift_buf(conn->read_buf, &conn->read_len, data, data_len);

                            struct request req;
                            if (parse_request(data, data_len, &req) != -1) {
                                printf("Request from %s:%d: %s %s %s\n",
                                       inet_ntoa(conn->addr.sin_addr),
                                       ntohs(conn->addr.sin_port),
                                       req.method, req.path, req.version);
                                gen_response(conn->write_buf, &conn->write_len);

                                conn->state = CONN_WRITING;

                                // Switch to writing mode
                                struct epoll_event ev = {
                                    .data.ptr = conn,
                                    .events = EPOLLOUT,
                                };
                                if (epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev) == -1) {
                                    perror("epoll_ctl");
                                    return -1;
                                }
                            } else {
                                printf("Failed to parse request from %s:%d: %.*s\n",
                                       inet_ntoa(conn->addr.sin_addr),
                                       ntohs(conn->addr.sin_port),
                                       (int)(data_len - needle_size), data);
                                conn->state = CONN_READING;
                                conn->read_len = 0;
                                break;
                            }
                        }
                    }
                    if (bytes == 0) {
                        conn->state = CONN_CLOSING;
                        close (conn->fd);
                        num_clients--;
                        printf("Client disconnected: %s:%d (%d)\n",
                               inet_ntoa(conn->addr.sin_addr),
                               ntohs(conn->addr.sin_port), num_clients);
                    } else if (bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("read");
                        return -1;
                    }
                } else if (conn->state == CONN_WRITING) {
                    while (conn->write_offset < conn->write_len) {
                        ssize_t bytes = write(conn->fd,
                                              conn->write_buf + conn->write_offset,
                                              conn->write_len - conn->write_offset);
                        if (bytes > 0) {
                            conn->write_offset += bytes;
                        }
                        if (bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                            perror("write");
                            return -1;
                        }
                    }
                    if (conn->write_offset == conn->write_len) {
                        conn->write_offset = 0;
                        conn->write_len = 0;
                        conn->state = CONN_READING;
                    }
                }
            }
        }
    }

    return 0;
}

void sig_handler(int signum) {
    switch (signum) {
        case SIGTERM:
            sigterm_received = 1;
            break;
        default:
            break;
    }
}