#ifndef RESPONSE_H
#define RESPONSE_H

#include "parser.h"
#include <stddef.h>
#include <string.h>

void gen_response(struct request req, char *buf, size_t *len);

#endif