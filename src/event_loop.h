#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "connection.h"
#include <stdint.h>
#include <signal.h>

extern volatile sig_atomic_t keep_running;
extern volatile sig_atomic_t keep_accepting;

int add_conn(int epfd, uint32_t events, void *data, int fd);

int start_event_loop(int epfd, struct connection *server_conn);

void sig_handler(int signum);

#endif