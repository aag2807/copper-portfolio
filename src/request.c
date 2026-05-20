#include "request.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void request_init(Request* req, const char* raw)
{
    memset(req, 0, sizeof(Request));

    req->method = str_new();
    req->path = str_new();
    req->query = str_new();
    req->version = str_new();
    req->body = str_new();
    req->client_ip = str_new();

    // Parse the request line: "GET /path?q=1 HTTP/1.1\r\n"
    const char* p = raw;

    // Method
    while (*p && *p != ' ')
    {
        str_appendf(&req->method, "%c", *p++);
    }

    if (*p)
    {
        p++; // skip space
    }
    //
    // Path with optional query string
    while (*p && *p != ' ' && *p != '?')
    {
        str_appendf(&req->path, "%c", *p++);
    }

    if (*p == '?')
    {
        p++; // skip '?'
        while (*p && *p != ' ')
        {
            str_appendf(&req->query, "%c", *p++);
        }
    }

    if (*p)
    {
        p++; // skip space
    }

    // Version
    while (*p && *p != '\r' && *p != '\n')
    {
        str_appendf(&req->version, "%c", *p++);
    }

    if (*p == '\r')
    {
        p++;
    }
    if (*p == '\n')
    {
        p++;
    }

    // Headers
    req->headers.cap = 16;
    req->headers.keys = calloc(req->headers.cap, sizeof(char*));
    req->headers.values = calloc(req->headers.cap, sizeof(char*));
    while (*p && *p != '\r' && p[1] && !(p[0] == '\r' && p[1] == '\n'))
    {
        if (*p == ' ' || *p == '\t')
        {
            p++;
            continue;
        }
        char key[256] = {0};
        char val[1024] = {0};
        int i = 0;
        //
        // Header name
        while (*p && *p != ':')
        {
            key[i++] = *p++;
        }
        if (*p == ':')
        {
            p++;
        }
        while (*p == ' ' || *p == '\t')
        {
            p++; // skip leading whitespace
        }
        i = 0;

        // Read the header value up to CRLF.
        while (*p && *p != '\r' && *p != '\n' && i + 1 < (int)sizeof(val))
        {
            val[i++] = *p++;
        }
        val[i] = '\0';

        if (*p == '\r') p++;
        if (*p == '\n') p++;

        if (req->headers.count >= req->headers.cap)
        {
            req->headers.cap *= 2;
            req->headers.keys = realloc(req->headers.keys, req->headers.cap * sizeof(char*));
            req->headers.values = realloc(req->headers.values, req->headers.cap * sizeof(char*));
        }

        req->headers.keys[req->headers.count] = strdup(key);
        req->headers.values[req->headers.count] = strdup(val);
        req->headers.count++;
    }

    // Skip empty line separating headers from body
    if (*p == '\r')
    {
        p++;
    }

    if (*p == '\n')
    {
        p++;
    }

    // Body
    const char* cl = request_header(req, "Content-Length");
    if (cl)
    {
        int len = atoi(cl);
        for (int i = 0; i < len && *p; i++)
        {
            str_appendf(&req->body, "%c", *p++);
        }
    }
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
