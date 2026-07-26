#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <sys/types.h>

#define BUF_SIZE 8192

struct read {
    char buf[BUF_SIZE];
    size_t len;
};

struct write {
    char buf[BUF_SIZE];
    size_t len;
    off_t offset;

    int file_fd;
    off_t file_offset;
};

// A movable slice of a string of bytes
struct slice {
    const char *data;
    size_t len;
};

int read_until(struct slice *buf, const struct slice *delim, struct slice *out);

int peek_until(const struct slice *buf, const struct slice *delim, struct slice *out);

int read_line(struct slice *buf, struct slice *out);
int read_word(struct slice *buf, struct slice *out);

#endif