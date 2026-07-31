#ifndef RESPONSE_H
#define RESPONSE_H

#include <stddef.h>

struct request;
struct write;

#define CODE_OK "200 OK"
#define CODE_NOT_FOUND "404 Not Found"
#define CODE_METHOD_NOT_ALLOWED "405 Method Not Allowed"

struct mime_map {
    const char *extension;
    const char *mime_type;
};

extern struct mime_map mime_registry[];

const char *get_mime_type_by_extension(const char *filename);

void prepare_headers(struct request *req, struct write *write);

#endif