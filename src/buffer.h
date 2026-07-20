#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

size_t find_eoh(char *buf, size_t buf_len);

int shift_buf(char *buf, size_t *buf_len, char *data, size_t data_len);

#endif