#include "connection.h"

const char *CONN_READING_STR = "CONN_READING";
const char *CONN_WRITING_STR = "CONN_WRITING";
const char *CONN_PARSING_STR = "CONN_PARSING";
const char *CONN_CLOSING_STR = "CONN_CLOSING";

void client_init(int fd, struct sockaddr_in addr, struct connection *conn) {
    conn->fd = fd;
    conn->addr = addr;
    conn->read.len = 0;
    conn->write.len = 0;
    conn->write.offset = 0;
    conn->write.file = NULL;
    conn->state = CONN_READING;
}

const char *state_str(struct connection *conn) {
    switch (conn->state) {
    case CONN_READING:
        return CONN_READING_STR;
    case CONN_WRITING:
        return CONN_WRITING_STR;
    case CONN_PARSING:
        return CONN_PARSING_STR;
    case CONN_CLOSING:
        return CONN_CLOSING_STR;
    default:
        return "\0";
    }
}