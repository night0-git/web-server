#include "response.h"

void gen_response(char *buf, size_t *len) {
    const char *res = "Hello, World!\n";

    *len = strlen(res);
    memcpy(buf, res, *len);
}