#include "response.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

struct mime_map mime_registry[] = {
    {".html", "text/html"},
    {".css",  "text/css"},
    {".png",  "image/png"},
    {".jpg",  "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".ico", "image/x-icon"},
    {".pdf",  "application/pdf"}
};

const char *get_mime_type_by_extension(const char *filename) {
    const char *default_type = "application/octet-stream";
    const char *dot = strrchr(filename, '.');
    if (!dot) {
        return default_type;
    }

    int total_types = sizeof(mime_registry) / sizeof(struct mime_map);
    for (int i = 0; i < total_types; i++) {
        if (strcasecmp(dot, mime_registry[i].extension) == 0) {
            return mime_registry[i].mime_type;
        }
    }

    return default_type;
}

void prepare_headers(struct request *req, struct write *write) {
    int is_root_req = strcmp(req->path, "/") == 0;

    const char *status = CODE_OK;
    char path[256];

    if (is_root_req) {
        snprintf(path, sizeof(path), "%s%s", SERVE_FILE_ROOT, "/index.html");
    } else {
        snprintf(path, sizeof(path), "%s%s", SERVE_FILE_ROOT, req->path);
    }

    const char *content_type = get_mime_type_by_extension(path);

    if (strcmp(req->method, "GET") == 0) {
        write->file_fd = open(path, O_RDONLY);
        if (write->file_fd != -1) {
            struct stat st;
            fstat(write->file_fd, &st);
            write->file_size = st.st_size;
        } else {
            status = CODE_NOT_FOUND;
        }
    } else {
        status = CODE_METHOD_NOT_ALLOWED;
    }

    int written = snprintf(write->buf, sizeof(write->buf),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %jd\r\n"
        "\r\n",
        status, content_type, (intmax_t)write->file_size);

    write->len = written;
}