#include "response.h"

#include "str.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#define sock_write(fd, buf, n) send((SOCKET)(fd), (const char*)(buf), (int)(n), 0)
typedef int ssize_t;
#else
#include <unistd.h>
#define sock_write(fd, buf, n) write((fd), (buf), (n))
#endif

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

void response_redirect_permanent(Response* res, const char* url)
{
    res->status_code = 301;
    str_free(&res->status_text);
    res->status_text = str_from("Moved Permanently");
    response_header(res, "Location", url);
}

void response_flush(Response* res, int fd)
{
    // Build the headers as a single buffer (status line + headers + blank line).
    String buf = str_new();
    str_appendf(&buf, "HTTP/1.1 %d %s\r\n", res->status_code, str_cstr(&res->status_text));
    str_appendf(&buf, "Content-Length: %zu\r\n", res->body.len);
    str_append(&buf, "Connection: close\r\n");
    str_append(&buf, str_cstr(&res->headers));
    str_append(&buf, "\r\n");

    // Write headers to socket.
    const char* head_data = str_cstr(&buf);
    size_t head_remaining = buf.len;
    while (head_remaining > 0)
    {
        ssize_t n = sock_write(fd, head_data, head_remaining);
        if (n <= 0)
            break;
        head_data += n;
        head_remaining -= n;
    }

    // Write body separately so binary payloads (PDFs, fonts, images) are not
    // truncated at the first NUL byte. We use res->body.len, not strlen.
    const char* body_data = str_cstr(&res->body);
    size_t body_remaining = res->body.len;
    while (body_remaining > 0)
    {
        ssize_t n = sock_write(fd, body_data, body_remaining);
        if (n <= 0)
            break;
        body_data += n;
        body_remaining -= n;
    }

    str_free(&buf);
}

void response_cleanup(Response* res)
{
    str_free(&res->status_text);
    str_free(&res->headers);
    str_free(&res->body);
}
