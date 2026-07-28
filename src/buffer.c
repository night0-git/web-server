#include "buffer.h"
#include <stddef.h>
#include <string.h>
#include <unistd.h>

int close_file(struct write *write) {
    if (write->file_fd != -1) {
        if (close(write->file_fd) == -1) {
            perror("close");
            return -1;
        }
        write->file_fd = -1;
        write->file_offset = 0;
        write->file_size = 0;

        return 0;
    }
    return 0;
}

int read_until(struct slice *buf, const struct slice *delim, struct slice *out) {
    if (peek_until(buf, delim, out) == -1) {
        return -1;
    }

    buf->data += out->len + delim->len;
    buf->len  -= out->len + delim->len;

    return 0;
}

int peek_until(const struct slice *buf, const struct slice *delim, struct slice *out) {
    const char *d = memmem(buf->data, buf->len, delim->data, delim->len);
    if (!d) {
        return -1;
    }
    out->data = buf->data;
    out->len = d - buf->data;
    return 0;
}

int read_line(struct slice *buf, struct slice *out) {
    struct slice delim = { "\r\n", 2 };
    return read_until(buf, &delim, out);
}

int read_word(struct slice *buf, struct slice *out) {
    struct slice delim = { " ", 1 };
    return read_until(buf, &delim, out);
}
