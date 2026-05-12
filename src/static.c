#include "static.h"

#include "request.h"
#include "response.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATIC_PREFIX "/static/"
#define STATIC_ROOT "public"
#define MAX_FILE_BYTES (8 * 1024 * 1024)

static const char* content_type_for(const char* path)
{
    const char* dot = strrchr(path, '.');
    if (!dot)
        return "application/octet-stream";
    if (!strcmp(dot, ".css"))
        return "text/css; charset=utf-8";
    if (!strcmp(dot, ".js"))
        return "application/javascript; charset=utf-8";
    if (!strcmp(dot, ".html"))
        return "text/html; charset=utf-8";
    if (!strcmp(dot, ".json"))
        return "application/json";
    if (!strcmp(dot, ".svg"))
        return "image/svg+xml";
    if (!strcmp(dot, ".png"))
        return "image/png";
    if (!strcmp(dot, ".jpg") || !strcmp(dot, ".jpeg"))
        return "image/jpeg";
    if (!strcmp(dot, ".gif"))
        return "image/gif";
    if (!strcmp(dot, ".ico"))
        return "image/x-icon";
    if (!strcmp(dot, ".woff"))
        return "font/woff";
    if (!strcmp(dot, ".woff2"))
        return "font/woff2";
    return "application/octet-stream";
}

int static_middleware(Request* req, Response* res, MiddlewareNode* self, Router* router)
{
    const char* path = str_cstr(&req->path);

    if (strncmp(path, STATIC_PREFIX, strlen(STATIC_PREFIX)) != 0)
    {
        return middleware_next(self, req, res, router);
    }

    const char* rel = path + strlen(STATIC_PREFIX);

    // Reject path traversal and absolute escapes
    if (rel[0] == '\0' || rel[0] == '/' || strstr(rel, "..") != NULL)
    {
        response_status(res, 403, "Forbidden");
        response_html(res, "<h1>403 - Forbidden</h1>");
        return 1;
    }

    char full[1024];
    int n = snprintf(full, sizeof(full), "%s/%s", STATIC_ROOT, rel);
    if (n < 0 || n >= (int)sizeof(full))
    {
        response_status(res, 414, "URI Too Long");
        response_html(res, "<h1>414</h1>");
        return 1;
    }

    FILE* f = fopen(full, "rb");
    if (!f)
    {
        response_status(res, 404, "Not Found");
        response_html(res, "<h1>404 - Not Found</h1>");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0 || size > MAX_FILE_BYTES)
    {
        fclose(f);
        response_status(res, 413, "Payload Too Large");
        response_html(res, "<h1>413</h1>");
        return 1;
    }

    char* buf = malloc((size_t)size);
    if (!buf)
    {
        fclose(f);
        response_status(res, 500, "Internal Server Error");
        return 1;
    }

    size_t read_n = fread(buf, 1, (size_t)size, f);
    fclose(f);

    response_bytes(res, content_type_for(rel), buf, read_n);
    free(buf);
    return 1;
}
