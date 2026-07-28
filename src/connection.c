#include "connection.h"
#include <string.h>

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

int add_active_fd(int fd, int *pool, int *curr_len, int max_fds) {
    if (*curr_len >= max_fds) {
        return -1;
    }
    pool[*curr_len] = fd;
    (*curr_len)++;
    return 0;
}

int remove_active_fd(int fd, int *pool, int *curr_len) {
    int pos = -1;
    for (int i = 0; i < *curr_len; i++) {
        if (pool[i] == fd) {
            pos = i;
            break;
        }
    }
    if (pos == -1) {
        return -1;
    }
    // Slide back 1 to fill the gap
    memmove(pool + pos, pool + pos + 1, (*curr_len - pos - 1) * sizeof(int));
    (*curr_len)--;
    return 0;
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