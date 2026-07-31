#ifndef LOG_H
#define LOG_H

#include "config.h"
#include <stdio.h>
#include <string.h>

#define LOG_INFO(fmt, ...) \
    fprintf(stdout, "[INFO]  " fmt "\n", ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    fprintf(stderr, "[WARN]  " fmt "\n", ##__VA_ARGS__)

#define LOG_ERR(fmt, ...) \
    fprintf(stderr, "[ERR]   " fmt "\n", ##__VA_ARGS__)

#define LOG_DBG(fmt, ...) \
    do { if (sv_conf.verbose >= 1) fprintf(stdout, "[DBG]   " fmt "\n", ##__VA_ARGS__); } while (0)

#define LOG_TRACE(fmt, ...) \
    do { if (sv_conf.verbose >= 2) fprintf(stdout, "[TRACE] " fmt "\n", ##__VA_ARGS__); } while (0)

// Print a buffer that contains \r\n-delimited lines, each prefixed with [TRACE]
static inline void log_trace_headers(const char *buf, size_t len) {
    if (sv_conf.verbose < 2) return;

    const char *pos = buf;
    const char *end = buf + len;
    while (pos < end) {
        const char *eol = memmem(pos, end - pos, "\r\n", 2);
        int line_len = eol ? (int)(eol - pos) : (int)(end - pos);
        fprintf(stdout, "[TRACE] %.*s\n", line_len, pos);
        pos += line_len + (eol ? 2 : 0);
    }
}

#endif
