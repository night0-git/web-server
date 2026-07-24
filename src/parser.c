#include "parser.h"
#include "buffer.h"
#include <string.h>
#include <stdlib.h>

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
    struct slice b = { buf, buf_len };

    struct slice line;
    if (read_line(&b, &line) == -1) {
        return -1;
    }

    struct slice method;
    if (read_word(&line, &method) == -1) {
        return -1;
    }
    memcpy(req->method, method.data, method.len);
    req->method[method.len] = '\0';

    struct slice path;
    if (read_word(&line, &path) == -1) {
        return -1;
    }
    memcpy(req->path, path.data, path.len);
    req->path[path.len] = '\0';

    memcpy(req->version, line.data, line.len);
    req->version[line.len] = '\0';

    req->content_len = 0;
    struct slice pair_delim = { ": ", 2 };
    while (read_line(&b, &line) != -1) {
        if (line.len == 0) {
            break;
        }

        struct slice key;
        if (read_until(&line, &pair_delim, &key) == -1) {
            return -1;
        }
        struct slice val = line;

        if (memcmp(key.data, "Content-Length", 14) == 0) {
            if ((req->content_len = strtoul(val.data, NULL, 10)) == 0) {
                return -1;
            }
            break;
        }
    }

    return 0;
}