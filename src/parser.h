#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

struct request {
    char method[16];
    char path[256];
    char version[16];
};

int shift_buf(char *buf, size_t *buf_len, char *data, size_t data_len);

int parse_request(const char *buf, size_t buf_len, struct request *req);

#endif