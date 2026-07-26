#ifndef RESPONSE_H
#define RESPONSE_H

#include "connection.h"
#include "parser.h"
#include <stddef.h>
#include <string.h>

void prepare_response(struct request *req, struct write *write);

#endif