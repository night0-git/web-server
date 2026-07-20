#include "buffer.h"
#include <stdint.h>
#include <string.h>

size_t find_eoh(char *buf, size_t buf_len) {
    if (buf_len < 4) {
        return SIZE_MAX;
    }
    for (size_t i = 0; i < buf_len - 3; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' &&
            buf[i + 2] == '\r' && buf[i + 3] == '\n') {
            return i + 4;
        }
    }
    return SIZE_MAX;
}

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