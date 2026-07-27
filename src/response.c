#include "response.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

void prepare_response(struct request *req, struct write *write) {
    char path[256];
    snprintf(path, sizeof(path), "%s%s", SERVE_FILE_ROOT, req->path);

    const char *status = CODE_OK;
    FILE *f = NULL;
    size_t content_len = 0;

    if (strcmp(req->method, "GET") == 0) {
        if (strcmp(req->path, "/") != 0) {
            write->file = fopen(path, "rb");
            f = write->file;
            if (f) {
                struct stat st;
                fstat(fileno(f), &st);
                content_len = st.st_size;
            } else {
                status = CODE_NOT_FOUND;
            }
        }
    } else {
        status = CODE_METHOD_NOT_ALLOWED;
    }

    if (strcmp(req->path, "/") == 0) {
        content_len = sizeof(ROOT_RESPONSE) - 1;
    }

    int written = snprintf(write->buf, sizeof(write->buf),
        "HTTP/1.1 %s\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "\r\n",
        status, content_len);
    if (strcmp(req->path, "/") == 0) {
        written += snprintf(write->buf + written, sizeof(write->buf) - written, ROOT_RESPONSE);
    }

    // Append file content to response
    if (f) {
        size_t bytes_read = fread(write->buf + written, 1, sizeof(write->buf) - written, f);
        if (bytes_read > 0) {
            written += (int)bytes_read;
        } else if (feof(f)) {
            close_file(write);
        } else {
            perror("fread");
            close_file(write);
        }
    }

    write->len = written;
}