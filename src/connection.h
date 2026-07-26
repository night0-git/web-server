#ifndef CONNECTION_H
#define CONNECTION_H

#include "buffer.h"
#include <netinet/in.h>

enum conn_state {
    CONN_READING,
    CONN_PARSING,
    CONN_WRITING,
    CONN_CLOSING,
};

struct connection {
    int fd;
    struct sockaddr_in addr;

    struct read read;
    struct write write;

    enum conn_state state;
};

void client_init(int fd, struct sockaddr_in addr, struct connection *conn);

#endif