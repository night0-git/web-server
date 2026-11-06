#ifndef LOG_H
#define LOG_H

#include "config.h"
#include <stdio.h>

#define LOG_INFO(fmt, ...) \
    fprintf(stdout, "[INFO]  " fmt "\n", ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    fprintf(stderr, "[WARN]  " fmt "\n", ##__VA_ARGS__)

#define LOG_ERR(fmt, ...) \
    fprintf(stderr, "[ERR]   " fmt "\n", ##__VA_ARGS__)

#define LOG_DBG(fmt, ...) \
    do { if (sv_conf.verbose >= 1) fprintf(stdout, "[DBG]   " fmt "\n", ##__VA_ARGS__); } while (0)

#endif
