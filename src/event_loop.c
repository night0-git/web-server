#include "event_loop.h"
#include "connection.h"
#include "server.h"
#include "parser.h"
#include "response.h"
#include "log.h"
#include <endian.h>
#include <linux/limits.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys/sendfile.h>

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

int close_conn(struct connection *conn) {
    conn->state = CONN_CLOSING;
    if (close(conn->fd) == -1) {
        perror("close");
        return -1;
    }
    if (close_file(&conn->write) == -1) {
        perror("close_file");
        return -1;
    }
    return 0;
}

int set_conn_events_flag(int epfd, struct connection *conn, uint32_t events) {
    struct epoll_event ev = {
        .data.ptr = conn,
        .events = events,
    };
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &ev) == -1) {
        perror("epoll_ctl");
        return -1;
    }
    return 0;
}

int close_conn_and_remove_fd(struct connection *conn, int *active_fds, int *num_clients) {
    if (close_conn(conn) == -1) {
        perror("close_conn");
        return -1;
    }
    if (remove_active_fd(conn->fd, active_fds, num_clients) == -1) {
        perror("remove_active_fd");
        return -1;
    }
    return 0;
}

// Handle one read event, a return value of -1 will terminate the server
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
                LOG_DBG("%s %s %s from %s:%d, Content-Length: %zu",
                        req.method, req.path, req.version,
                        inet_ntoa(conn->addr.sin_addr),
                        ntohs(conn->addr.sin_port), req.content_len);
                prepare_headers(&req, &conn->write);

                conn->state = CONN_WRITING_HEADERS;
                if (set_conn_events_flag(epfd, conn, EPOLLOUT) == -1) {
                    perror("set_conn_events_flag");
                    return -1;
                }

                break;
            } else {
                LOG_WARN("failed to parse request from %s:%d",
                         inet_ntoa(conn->addr.sin_addr),
                         ntohs(conn->addr.sin_port));
                conn->state = CONN_READING;
                conn->read.len = 0;
            }
        }
    }

    if (bytes == 0) {
        if (close_conn_and_remove_fd(conn, active_fds, num_clients) == -1) {
            perror("close_conn_and_remove_fd");
            return -1;
        }
        LOG_DBG("client disconnected: %s:%d (%d)",
                inet_ntoa(conn->addr.sin_addr),
                ntohs(conn->addr.sin_port), *num_clients);
        return 0;
    } else if (bytes == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;   // wait for next EPOLLIN
        } else if (errno == ECONNRESET) {
            if (close_conn_and_remove_fd(conn, active_fds, num_clients) == -1) {
                perror("close_conn_and_remove_fd");
                return -1;
            }
            LOG_DBG("client disconnected: %s:%d (%d)",
                    inet_ntoa(conn->addr.sin_addr),
                    ntohs(conn->addr.sin_port), *num_clients);
        } else {
            // Print the previous read error
            perror("read");
            if (close_conn_and_remove_fd(conn, active_fds, num_clients) == -1) {
                perror("close_conn_and_remove_fd");
                return -1;
            }
            LOG_WARN("client closed due to read error: %s:%d (%d)",
                     inet_ntoa(conn->addr.sin_addr),
                     ntohs(conn->addr.sin_port), *num_clients);
        }
        return 0;
    }

    if (sizeof(conn->read.buf) <= conn->read.len) {
        // Reset the whole read buffer if we don't find a request
        LOG_WARN("buffer full, dropping data");
        if (conn->state == CONN_PARSING) {
            conn->state = CONN_READING;
        }
        conn->read.len = 0;
    }

    return 0;
}

// Handle one write event (headers), a return value of -1 will terminate the server
int conn_write_headers(struct connection *conn, int epfd, int *active_fds, int *num_clients) {
    if (conn->write.offset == 0) {
        const char *delim = memmem(conn->write.buf, conn->write.len, "\r\n\r\n", 4);
        if (delim) {
            log_trace_headers(conn->write.buf, delim - conn->write.buf);
        }

        // Enable TCP_CORK to coalesce header write calls
        if (setsockopt(conn->fd, IPPROTO_TCP, TCP_CORK, &(int){1}, sizeof(int)) == -1) {
            perror("setsockopt");
            return -1;
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
            } else {
                if (errno == ECONNRESET || errno == EPIPE) {
                    if (close_conn_and_remove_fd(conn, active_fds, num_clients) == -1) {
                        perror("close_conn_and_remove_fd");
                        return -1;
                    }
                    LOG_DBG("client disconnected: %s:%d (%d)",
                            inet_ntoa(conn->addr.sin_addr),
                            ntohs(conn->addr.sin_port), *num_clients);
                } else {
                    perror("write");
                    if (close_conn_and_remove_fd(conn, active_fds, num_clients) == -1) {
                        perror("close_conn_and_remove_fd");
                        return -1;
                    }
                    LOG_WARN("client closed due to write error: %s:%d (%d)",
                             inet_ntoa(conn->addr.sin_addr),
                             ntohs(conn->addr.sin_port), *num_clients);
                }
                return 0;
            }
        }
    }

    if (conn->write.offset == conn->write.len) {
        conn->write.offset = 0;
        conn->write.len = 0;

        if (conn->write.file_fd != -1) {
            conn->state = CONN_WRITING_FILE;
        } else {
            conn->state = CONN_READING;
            if (set_conn_events_flag(epfd, conn, EPOLLIN) == -1) {
                perror("set_conn_events_flag");
                return -1;
            }
        }
    }

    return 0;
}

// Handle one write event (file), a return value of -1 will terminate the server
int conn_write_file(struct connection *conn, int epfd, int *active_fds, int *num_clients) {
    if (conn->write.file_fd != -1) {
        while (conn->write.file_offset < conn->write.file_size) {
            ssize_t bytes_sent = sendfile(conn->fd, conn->write.file_fd,
                                          &conn->write.file_offset,
                                          conn->write.file_size - conn->write.file_offset);
            if (bytes_sent == -1) {
                if (errno == EAGAIN) {
                    return 0;
                }

                perror("sendfile");
                if (close_conn_and_remove_fd(conn, active_fds, num_clients) == -1) {
                    perror("close_conn_and_remove_fd");
                    return -1;
                }
                LOG_WARN("client closed due to write error: %s:%d (%d)",
                         inet_ntoa(conn->addr.sin_addr),
                         ntohs(conn->addr.sin_port), *num_clients);

                return 0;
            }
        }

        if (conn->write.file_offset == conn->write.file_size) {
            // Disable TCP_CORK after file write
            if (setsockopt(conn->fd, IPPROTO_TCP, TCP_CORK, &(int){0}, sizeof(int)) == -1) {
                perror("setsockopt");
                return -1;
            }

            close_file(&conn->write);

            conn->state = CONN_READING;
            if (set_conn_events_flag(epfd, conn, EPOLLIN) == -1) {
                perror("set_conn_events_flag");
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

                while (1) {
                    if (num_clients >= MAX_FDS) {
                        break;
                    }

                    client_addr_len = sizeof(client_addr);
                    int conn = accept(server_conn->fd, (struct sockaddr *)&client_addr, &client_addr_len);

                    if (conn == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        if (errno == EMFILE) {
                            LOG_WARN("file descriptor limit reached for this process");
                            break;
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

                    // Add client socket to epoll instance
                    if (add_conn(epfd, EPOLLIN, &clients[conn], conn) == -1) {
                        perror("add_conn");
                        return -1;
                    }

                    if (add_active_fd(conn, active_fds, &num_clients, MAX_FDS) == -1) {
                        // Theoretically this should not happen because we exit earlier
                        if (epoll_ctl(epfd, EPOLL_CTL_DEL, conn, NULL) == -1) {
                            perror("epoll_ctl");
                            return -1;
                        }
                        if (close(conn) == -1) {
                            perror("close");
                            return -1;
                        }

                        continue;
                    }

                    LOG_DBG("client connected: %s:%d (%d)",
                            inet_ntoa(client_addr.sin_addr),
                            ntohs(client_addr.sin_port), num_clients);
                }
            } else {
                // Handle client
                struct connection *conn = (struct connection *)events[i].data.ptr;
                if (conn->state == CONN_READING) {
                    if (conn_read(conn, epfd, active_fds, &num_clients) == -1) {
                        perror("conn_read");
                        return -1;
                    }
                } else if (conn->state == CONN_WRITING_HEADERS) {
                    if (conn_write_headers(conn, epfd, active_fds, &num_clients) == -1) {
                        perror("conn_write_headers");
                        return -1;
                    }
                } else if (conn->state == CONN_WRITING_FILE) {
                    if (conn_write_file(conn, epfd, active_fds, &num_clients) == -1) {
                        perror("conn_write_file");
                        return -1;
                    }
                }
            }
        }
    }

    if (sigint_received) {
        for (int i = 0; i < num_clients; i++) {
            int fd = active_fds[i];
            if (close_conn(&clients[fd]) == -1) {
                perror("close_conn");
            }
            LOG_DBG("client closed: %s:%d (%d)",
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