#ifndef RESPONSE_H
#define RESPONSE_H

#include "connection.h"
#include "parser.h"
#include <stddef.h>
#include <string.h>

#define SERVE_FILE_ROOT "./files"

#define CODE_OK "200 OK"
#define CODE_NOT_FOUND "404 Not Found"
#define CODE_METHOD_NOT_ALLOWED "405 Method Not Allowed"

#define RESPONSE_TEXT_PLAIN "text/plain"
#define RESPONSE_TEXT_HTML "text/html"

void prepare_response(struct request *req, struct write *write);

#endif