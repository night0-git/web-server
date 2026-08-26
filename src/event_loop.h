#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "connection.h"
#include <stdint.h>
#include <signal.h>

#define MAX_EVENTS 256
#define MAX_FDS 65535

extern volatile sig_atomic_t sigterm_received;
extern volatile sig_atomic_t sigint_received;

int add_conn(int epfd, uint32_t events, void *data, int fd);

void sig_handler(int signum);

struct epoll_instance {
    int epfd;
    struct connection *server_conn;
};

int start_event_loop(const struct epoll_instance *epoll_inst);

int init_epoll_instance(struct epoll_instance *epoll_inst,
                        struct connection *server_conn);

#endif