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

volatile sig_atomic_t sigterm_received = 0;
volatile sig_atomic_t sigint_received = 0;

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

int close_conn(struct connection *conn, int num_clients) {
    conn->state = CONN_CLOSING;
    if (close(conn->fd) == -1) {
        perror("close");
        return -1;
    }
    close_file(&conn->write);
    printf("Client disconnected: %s:%d (%d)\n",
           inet_ntoa(conn->addr.sin_addr),
           ntohs(conn->addr.sin_port), num_clients - 1);
    return 0;
}

// Handle one read connection
int conn_read(struct connection *conn, int epfd, int *active_fds, int *num_clients) {
    ssize_t bytes;
    while (sizeof(conn->read.buf) - conn->read.len > 0) {
        bytes = read(conn->fd, conn->read.buf + conn->read.len, sizeof(conn->read.buf) - conn->read.len);
        if (bytes <= 0) {
            break;
        }

        conn->state = CONN_READING;
        conn->read.len += bytes;

        const char *eoh;
        const char *needle = "\r\n\r\n";
        size_t needle_size = strlen(needle);
        if ((eoh = memmem(conn->read.buf, conn->read.len, needle, needle_size))) {
            eoh += needle_size;

            conn->state = CONN_PARSING;

            char data[BUF_SIZE];
            size_t data_len = eoh - conn->read.buf;
            shift_buf(conn->read.buf, &conn->read.len, data, data_len);

            struct request req;
            if (parse_request(data, data_len, &req) != -1) {
                printf("Request from %s:%d:\n%s %s %s, Content-Length: %zu\n",
                       inet_ntoa(conn->addr.sin_addr),
                       ntohs(conn->addr.sin_port),
                       req.method, req.path, req.version, req.content_len);
                prepare_response(&req, &conn->write);

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
                conn->read.len = 0;
            }
        }
    }
    if (bytes == 0 || (bytes == -1 && errno == ECONNRESET)) {
        if (close_conn(conn, *num_clients) == -1) {
            perror("close_conn");
            return -1;
        }
        if (remove_active_fd(conn->fd, active_fds, num_clients) == -1) {
            perror("remove_active_fd");
            return -1;
        };
        return 0;
    } else if (bytes == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;   // wait for next EPOLLIN
        }

        perror("read");
        if (close_conn(conn, *num_clients) == -1) {
            perror("close_conn");
            return -1;
        }
        if (remove_active_fd(conn->fd, active_fds, num_clients) == -1) {
            perror("remove_active_fd");
            return -1;
        };

        return -1;
    }
    if (sizeof(conn->read.buf) <= conn->read.len) {
        // Reset the whole read buffer if we don't find a request
        printf("Buffer full, dropping data\n");
        if (conn->state == CONN_PARSING) {
            conn->state = CONN_READING;
        }
        conn->read.len = 0;
    }
    return 0;
}

// Handle one write connection
int conn_write(struct connection *conn, int epfd, int *active_fds, int *num_clients) {
    // Print only the headers
    if (memcmp(conn->write.buf + conn->write.offset, "HTTP/1.1", sizeof("HTTP/1.1") - 1) == 0) {
        const char *body = memmem(conn->write.buf, conn->write.len, "\r\n\r\n", 4);
        if (body) {
            printf("Response generated:\n%.*s\n", (int)(body - conn->write.buf), conn->write.buf);
        }
    }

    while (conn->write.offset < conn->write.len) {
        ssize_t bytes = write(conn->fd,
                              conn->write.buf + conn->write.offset,
                              conn->write.len - conn->write.offset);
        if (bytes > 0) {
            conn->write.offset += bytes;
        } else if (bytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;   // wait for next EPOLLOUT
            } else if (errno == ECONNRESET) {
                if (close_conn(conn, *num_clients) == -1) {
                    perror("close_conn");
                    return -1;
                }
                if (remove_active_fd(conn->fd, active_fds, num_clients) == -1) {
                    perror("remove_active_fd");
                    return -1;
                }
                return 0;
            }
            
            perror("write");
            if (close_conn(conn, *num_clients) == -1) {
                perror("close_conn");
                return -1;
            }
            if (remove_active_fd(conn->fd, active_fds, num_clients) == -1) {
                perror("remove_active_fd");
                return -1;
            };

            return -1;
        }
    }

    if (conn->write.offset == conn->write.len) {
        // Fetch more data from file if there is a file opened
        if (conn->write.file && !feof(conn->write.file)) {
            size_t bytes_read = fread(conn->write.buf, 1, sizeof(conn->write.buf), conn->write.file);
            if (bytes_read > 0) {
                conn->write.offset = 0;
                conn->write.len = bytes_read;
            } else if (feof(conn->write.file)) {
                close_file(&conn->write);
            } else {
                perror("fread");
                close_file(&conn->write);
                return -1;
            }
        } else {
            conn->write.offset = 0;
            conn->write.len = 0;

            conn->state = CONN_READING;

            // Switch to reading mode
            struct epoll_event ev = {
                .data.ptr = conn,
                .events = EPOLLIN,
            };
            if (epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev) == -1) {
                perror("epoll_ctl");
                return -1;
            }
        }
    }
    return 0;
}

int start_event_loop(int epfd, struct connection *server_conn) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    static struct connection clients[MAX_FDS];
    struct epoll_event events[MAX_EVENTS];

    // Track active fds (we simply use an array here)
    int active_fds[MAX_FDS];
    int num_clients = 0;

    while ((!sigterm_received || num_clients > 0) && !sigint_received) {
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
                // This is the last event on the listen socket if SIGTERM is received
                if (sigterm_received) {
                    if (close(server_conn->fd)) {
                        perror("close");
                        return -1;
                    }
                    continue;
                }

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
                client_init(conn, client_addr, &clients[conn]);

                // Try to add the fd before adding to epoll instance to check if we have any room left
                if (add_active_fd(conn, active_fds, &num_clients, MAX_FDS) == -1) {
                    int err = errno;
                    perror("add_active_fd");

                    if (close(conn) == -1) {
                        perror("close");
                    }

                    errno = err;
                    return -1;
                }

                // Add client socket to epoll instance
                if (add_conn(epfd, EPOLLIN, &clients[conn], conn) == -1) {
                    perror("add_conn");
                    return -1;
                }

                printf("Client connected: %s:%d (%d)\n",
                       inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port), num_clients);
            } else {
                // Handle client
                struct connection *conn = (struct connection *)events[i].data.ptr;
                if (conn->state == CONN_READING) {
                    if (conn_read(conn, epfd, active_fds, &num_clients) == -1) {
                        perror("conn_read");
                        return -1;
                    }
                } else if (conn->state == CONN_WRITING) {
                    if (conn_write(conn, epfd, active_fds, &num_clients) == -1) {
                        perror("conn_write");
                        return -1;
                    }
                }
            }
        }
    }

    if (sigint_received) {
        for (int i = 0; i < num_clients; i++) {
            int fd = active_fds[i];
            if (close(fd) == -1) {
                perror("close");
            }
            printf("Client closed: %s:%d (%d)\n",
                   inet_ntoa(clients[fd].addr.sin_addr),
                   ntohs(clients[fd].addr.sin_port), num_clients - i - 1);
        }
    }

    return 0;
}

void sig_handler(int signum) {
    switch (signum) {
        case SIGTERM:
            sigterm_received = 1;
            break;
        case SIGINT:
            sigint_received = 1;
            break;
        default:
            break;
    }
}