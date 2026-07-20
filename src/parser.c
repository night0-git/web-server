#include "parser.h"
#include <string.h>

int shift_buf(char *buf, size_t *buf_len, char *data, size_t data_len) {
    if (data_len > *buf_len) {
        return -1;
    }

    memcpy(data, buf, data_len);

    size_t shift = *buf_len - data_len;
    memmove(buf, buf + data_len, shift);
    *buf_len = shift;

    return 0;
}

int parse_request(const char *buf, size_t buf_len, struct request *req) {
    const char *crlf = memmem(buf, buf_len, "\r\n", 2);
    if (!crlf) {
        return -1;
    }

    const char *space1 = memchr(buf, ' ', buf_len);
    if (!space1 || space1 >= crlf) {
        return -1;
    }
    size_t method_len = space1 - buf;
    memcpy(req->method, buf, method_len);
    req->method[method_len] = '\0';

    const char *space2 = memchr(space1 + 1, ' ', buf_len - (space1 - buf) - 1);
    if (!space2 || space2 >= crlf) {
        return -1;
    }
    size_t path_len = space2 - space1 - 1;
    memcpy(req->path, space1 + 1, path_len);
    req->path[path_len] = '\0';

    size_t ver_len = crlf - space2 - 1;
    memcpy(req->version, space2 + 1, ver_len);
    req->version[ver_len] = '\0';

    return 0;
}