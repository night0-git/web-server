#include "connection.h"

void client_init(int fd, struct sockaddr_in addr, struct connection *conn) {
    conn->fd = fd;
    conn->addr = addr;
    conn->read.len = 0;
    conn->write.len = 0;
    conn->write.offset = 0;
    conn->write.file_fd = -1;
    conn->state = CONN_READING;
}