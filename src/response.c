#include "response.h"

#include "str.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

void response_init(Response* res, int fd)
{
    res->status_code = 200;
    res->status_text = str_from("OK");
    res->headers = str_new();
    res->body = str_new();
    res->fd = fd;
    res->headers_sent = 0;
}

void response_status(Response* res, int code, const char* text)
{
    res->status_code = code;
    str_free(&res->status_text);
    res->status_text = str_from(text);
}

void response_header(Response* res, const char* key, const char* value)
{
    str_appendf(&res->headers, "%s: %s\r\n", key, value);
}

void response_send(Response* res, const char* body)
{
    str_append(&res->body, body);
}

void response_json(Response* res, const char* json)
{
    response_header(res, "Content-Type", "application/json");
    str_append(&res->body, json);
}

void response_html(Response* res, const char* html)
{
    response_header(res, "Content-Type", "text/html; charset=utf-8");
    str_append(&res->body, html);
}

void response_bytes(Response* res, const char* content_type, const void* data, size_t len)
{
    response_header(res, "Content-Type", content_type);
    str_append_bytes(&res->body, data, len);
}

void response_redirect(Response* res, const char* url)
{
    res->status_code = 302;
    str_free(&res->status_text);
    res->status_text = str_from("Found");
    response_header(res, "Location", url);
}

void response_flush(Response* res, int fd)
{
    // Build the status line
    String buf = str_new();
    str_appendf(&buf, "HTTP/1.1 %d %s\r\n", res->status_code, str_cstr(&res->status_text));

    // Add content - length char cl[32];
    char cl[32];

    snprintf(cl, sizeof(cl), "%zu", res->body.len);
    str_appendf(&buf, "Content-Length: %s\r\n", cl);
    str_append(&buf, "Connection: close\r\n");

    // Add custom headers
    str_append(&buf, str_cstr(&res->headers));

    // End headers
    str_append(&buf, "\r\n");

    // Add body
    str_append(&buf, str_cstr(&res->body));

    // Write to socket
    const char* data = str_cstr(&buf);
    size_t remaining = buf.len;
    while (remaining > 0)
    {
        ssize_t n = write(fd, data, remaining);
        if (n <= 0)
            break;
        data += n;
        remaining -= n;
    }

    str_free(&buf);
}

void response_cleanup(Response* res)
{
    str_free(&res->status_text);
    str_free(&res->headers);
    str_free(&res->body);
}
