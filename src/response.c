#include "response.h"
#include <stdio.h>
#include <string.h>

void prepare_response(struct request *req, struct write *write) {
    char body[200];
    const char *status = "200 OK";

    if (strcmp(req->method, "GET") == 0) {
        if (strcmp(req->path, "/") == 0) {
            strcpy(body, "Hello, World!");
        } else if (strcmp(req->path, "/favicon.ico") == 0) {
            status = "404 Not Found";
            strcpy(body, "Favicon not found");
        } else {
            status = "404 Not Found";
            strcpy(body, "Page not found");
        }
    } else {
        status = "405 Method Not Allowed";
        strcpy(body, "Method Not Allowed");
    }

    int bytes_written = sprintf(write->buf,
        "HTTP/1.1 %s\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s",
        status, strlen(body), body);

    if (bytes_written > 0) {
        write->len = (size_t)bytes_written;
    } else {
        write->len = 0;
    }
}