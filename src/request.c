#include "request.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_token_char(unsigned char c)
{
    if (c <= 0x20 || c >= 0x7f) return 0;
    return strchr("()<>@,;:\\\"/[]?={}", c) == NULL;
}

static int push_header(Request* req, const char* key, size_t klen, const char* val, size_t vlen)
{
    if (req->headers.count >= REQ_MAX_HEADERS) return 431;
    if (req->headers.count >= req->headers.cap)
    {
        req->headers.cap *= 2;
        req->headers.keys = realloc(req->headers.keys, req->headers.cap * sizeof(char*));
        req->headers.values = realloc(req->headers.values, req->headers.cap * sizeof(char*));
    }
    req->headers.keys[req->headers.count] = strndup(key, klen);
    req->headers.values[req->headers.count] = strndup(val, vlen);
    req->headers.count++;
    return 0;
}

int request_init(Request* req, const char* raw, size_t raw_len)
{
    memset(req, 0, sizeof(Request));

    req->method = str_new();
    req->path = str_new();
    req->query = str_new();
    req->version = str_new();
    req->body = str_new();
    req->client_ip = str_new();

    req->headers.cap = 16;
    req->headers.keys = calloc(req->headers.cap, sizeof(char*));
    req->headers.values = calloc(req->headers.cap, sizeof(char*));

    const char* p = raw;
    const char* end = raw + raw_len;

    // Parse the request line: "GET /path?q=1 HTTP/1.1\r\n"
    // Method: uppercase token, bounded.
    const char* s = p;
    while (p < end && *p >= 'A' && *p <= 'Z' && (size_t)(p - s) < REQ_MAX_METHOD) p++;
    if (p == s || p >= end || *p != ' ') return 400;
    str_append_bytes(&req->method, s, (size_t)(p - s));
    p++; // skip space

    // Path with optional query string. Stops at SP, CR, LF; rejects controls.
    s = p;
    while (p < end && *p != ' ' && *p != '?' && *p != '\r' && *p != '\n')
    {
        unsigned char c = (unsigned char)*p;
        if (c < 0x20 || c == 0x7f) return 400;
        if ((size_t)(p - s) >= REQ_MAX_TARGET) return 414;
        p++;
    }
    if (p == s || *s != '/') return 400;
    str_append_bytes(&req->path, s, (size_t)(p - s));

    if (p < end && *p == '?')
    {
        p++; // skip '?'
        s = p;
        while (p < end && *p != ' ' && *p != '\r' && *p != '\n')
        {
            unsigned char c = (unsigned char)*p;
            if (c < 0x20 || c == 0x7f) return 400;
            if ((size_t)(p - s) >= REQ_MAX_TARGET) return 414;
            p++;
        }
        str_append_bytes(&req->query, s, (size_t)(p - s));
    }

    if (p >= end || *p != ' ') return 400;
    p++; // skip space

    // Version
    s = p;
    while (p < end && *p != '\r' && *p != '\n' && (size_t)(p - s) < REQ_MAX_VERSION) p++;
    if ((size_t)(p - s) < 8 || strncmp(s, "HTTP/1.", 7) != 0) return 400;
    str_append_bytes(&req->version, s, (size_t)(p - s));

    if (p < end && *p == '\r') p++;
    if (p >= end || *p != '\n') return 400;
    p++;

    // Headers: "Name: value\r\n" until an empty line.
    for (;;)
    {
        if (p >= end) break; // no blank line: headers-only request
        if (*p == '\n') { p++; break; }
        if (*p == '\r' && p + 1 < end && p[1] == '\n') { p += 2; break; }

        // Header name: token chars up to ':', bounded, never past CR/LF.
        s = p;
        while (p < end && *p != ':' && *p != '\r' && *p != '\n')
        {
            if (!is_token_char((unsigned char)*p)) return 400;
            if ((size_t)(p - s) >= REQ_MAX_HEADER_NAME) return 400;
            p++;
        }
        if (p >= end || *p != ':' || p == s) return 400; // no colon / empty name
        size_t klen = (size_t)(p - s);
        const char* key = s;
        p++; // skip ':'

        while (p < end && (*p == ' ' || *p == '\t')) p++;
        s = p;
        while (p < end && *p != '\r' && *p != '\n') p++;
        const char* vend = p;
        while (vend > s && (vend[-1] == ' ' || vend[-1] == '\t')) vend--;

        int rc = push_header(req, key, klen, s, (size_t)(vend - s));
        if (rc) return rc;

        if (p < end && *p == '\r') p++;
        if (p < end && *p == '\n') p++;
    }

    // Body
    const char* cl = request_header(req, "Content-Length");
    if (cl)
    {
        long len = request_parse_content_length(cl);
        if (len == -1) return 400;
        if (len == -2) return 413;
        size_t avail = (size_t)(end - p);
        size_t take = (size_t)len < avail ? (size_t)len : avail;
        str_append_bytes(&req->body, p, take);
    }
    return 0;
}

long request_parse_content_length(const char* v)
{
    if (!v || !*v) return -1;
    long n = 0;
    for (const char* q = v; *q; q++)
    {
        if (*q < '0' || *q > '9') return -1;
        n = n * 10 + (*q - '0');
        if (n > REQ_MAX_BODY) return -2;
    }
    return n;
}

int request_xff_client_ip(const char* xff, char* out, size_t cap)
{
    if (!out || cap == 0) return 0;
    out[0] = '\0';
    if (!xff || !*xff) return 0;

    // Cloud Run's front end appends the address it saw to the end of the list,
    // so the LAST entry is the only one a client cannot forge.
    const char* comma = strrchr(xff, ',');
    const char* s = comma ? comma + 1 : xff;
    while (*s == ' ' || *s == '\t') s++;
    const char* e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t')) e--;

    size_t n = (size_t)(e - s);
    if (n == 0 || n >= cap) return 0;
    for (size_t i = 0; i < n; i++)
    {
        char c = s[i];
        int ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || c == '.' || c == ':';
        if (!ok) return 0;
    }
    memcpy(out, s, n);
    out[n] = '\0';
    return 1;
}

int request_canonical_path(const char* path, String* out)
{
    // Collapse runs of '/' and drop a trailing '/' (except for the root).
    const char* p = path;
    int prev_slash = 0;
    for (; *p; p++)
    {
        if (*p == '/')
        {
            if (prev_slash) continue;
            prev_slash = 1;
        }
        else
        {
            prev_slash = 0;
        }
        str_append_bytes(out, p, 1);
    }
    if (out->len == 0) str_append(out, "/");
    if (out->len > 1 && out->data[out->len - 1] == '/')
    {
        out->len--;
        out->data[out->len] = '\0';
    }
    return strcmp(str_cstr(out), path) != 0;
}

void request_set_param(Request* req, const char* key, const char* value)
{
    if (req->params.count >= req->params.cap)
    {
        req->params.cap = req->params.cap ? req->params.cap * 2 : 8;
        req->params.keys = realloc(req->params.keys, req->params.cap * sizeof(char*));
        req->params.values = realloc(req->params.values, req->params.cap * sizeof(char*));
    }
    req->params.keys[req->params.count] = strdup(key);
    req->params.values[req->params.count] = strdup(value);
    req->params.count++;
}

const char* request_param(Request* req, const char* key)
{
    for (int i = 0; i < req->params.count; i++)
    {
        if (strcmp(req->params.keys[i], key) == 0)
            return req->params.values[i];
    }
    return NULL;
}

const char* request_header(Request* req, const char* key)
{
    for (int i = 0; i < req->headers.count; i++)
    {
        if (strcasecmp(req->headers.keys[i], key) == 0)
            return req->headers.values[i];
    }
    return NULL;
}

void request_cleanup(Request* req)
{
    str_free(&req->method);
    str_free(&req->path);
    str_free(&req->query);
    str_free(&req->version);
    str_free(&req->body);
    str_free(&req->client_ip);

    for (int i = 0; i < req->headers.count; i++)
    {
        free(req->headers.keys[i]);
        free(req->headers.values[i]);
    }

    free(req->headers.keys);
    free(req->headers.values);

    for (int i = 0; i < req->params.count; i++)
    {
        free(req->params.keys[i]);
        free(req->params.values[i]);
    }

    free(req->params.keys);
    free(req->params.values);
}
