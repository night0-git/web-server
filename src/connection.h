#ifndef CONNECTION_H
#define CONNECTION_H

#include <netinet/in.h>

#define BUF_SIZE 4096

enum conn_state {
    CONN_READING,
    CONN_PARSING,
    CONN_WRITING,
    CONN_CLOSING,
};

struct connection {
    int fd;
    struct sockaddr_in addr;

    char read_buf[BUF_SIZE];
    size_t read_len;

    char write_buf[BUF_SIZE];
    size_t write_len;
    size_t write_offset;

    enum conn_state state;
};

#endif