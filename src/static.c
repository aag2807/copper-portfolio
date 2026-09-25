#include "static.h"

#include "request.h"
#include "response.h"
#include "view.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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
    if (!strcmp(dot, ".js") || !strcmp(dot, ".mjs"))
        return "application/javascript; charset=utf-8";
    if (!strcmp(dot, ".wasm"))
        return "application/wasm";
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
    if (!strcmp(dot, ".pdf"))
        return "application/pdf";
    return "application/octet-stream";
}

// Does the If-None-Match header list this ETag (or "*")?
static int etag_matches(const char* inm, const char* etag)
{
    if (!inm || !*inm) return 0;
    if (strcmp(inm, "*") == 0) return 1;
    size_t elen = strlen(etag);
    const char* p = inm;
    while (*p)
    {
        while (*p == ' ' || *p == '\t' || *p == ',') p++;
        if (p[0] == 'W' && p[1] == '/') p += 2; // weak comparison
        const char* e = p;
        while (*e && *e != ',') e++;
        const char* t = e;
        while (t > p && (t[-1] == ' ' || t[-1] == '\t')) t--;
        if ((size_t)(t - p) == elen && strncmp(p, etag, elen) == 0) return 1;
        p = e;
    }
    return 0;
}

int static_send_file(Request* req, Response* res, const char* full, const char* content_type)
{
    struct stat st;
    if (stat(full, &st) != 0 || !S_ISREG(st.st_mode))
    {
        return 0;
    }

    if (st.st_size > MAX_FILE_BYTES)
    {
        response_status(res, 413, "Payload Too Large");
        response_html(res, "<h1>413</h1>");
        return 1;
    }

    char etag[64];
    snprintf(etag, sizeof(etag), "\"%llx-%llx\"", (unsigned long long)st.st_size, (unsigned long long)st.st_mtime);
    // Assets are not fingerprinted, so browsers must revalidate on every use;
    // the ETag keeps that to a cheap 304 when nothing changed.
    response_header(res, "Cache-Control", "public, no-cache");
    response_header(res, "ETag", etag);

    if (etag_matches(request_header(req, "If-None-Match"), etag))
    {
        response_status(res, 304, "Not Modified");
        return 1;
    }

    FILE* f = fopen(full, "rb");
    if (!f)
    {
        return 0;
    }

    size_t size = (size_t)st.st_size;
    char* buf = malloc(size ? size : 1);
    if (!buf)
    {
        fclose(f);
        response_status(res, 500, "Internal Server Error");
        return 1;
    }

    size_t read_n = fread(buf, 1, size, f);
    fclose(f);

    response_bytes(res, content_type, buf, read_n);
    free(buf);
    return 1;
}

int static_middleware(Request* req, Response* res, MiddlewareNode* self, Router* router)
{
    const char* path = str_cstr(&req->path);

    if (strncmp(path, STATIC_PREFIX, strlen(STATIC_PREFIX)) != 0)
    {
        return middleware_next(self, req, res, router);
    }

    const char* method = str_cstr(&req->method);
    if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)
    {
        response_status(res, 405, "Method Not Allowed");
        response_header(res, "Allow", "GET, HEAD");
        response_html(res, "<h1>405 - Method Not Allowed</h1>");
        return 1;
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

    // Missing files and directories both get the 404 page.
    if (!static_send_file(req, res, full, content_type_for(rel)))
    {
        render_not_found(res);
    }
    return 1;
}
