#ifndef RESPONSE_H
#define RESPONSE_H

#include "str.h"

typedef struct
{
    int status_code;
    String status_text;
    String headers; // built-up header string
    String body;
    int fd; // client socket
    int headers_sent;
    int omit_body; // HEAD: send headers (incl. Content-Length) but no body
} Response;

void response_init(Response* res, int fd);
void response_status(Response* res, int code, const char* text);
void response_header(Response* res, const char* key, const char* value);
void response_send(Response* res, const char* body);
void response_json(Response* res, const char* json);
void response_html(Response* res, const char* html);
void response_bytes(Response* res, const char* content_type, const void* data, size_t len);
void response_redirect(Response* res, const char* url);
void response_redirect_permanent(Response* res, const char* url);
// Copies the value of the first `key` header set so far (case-insensitive)
// into `out`. Returns 1 if found, 0 otherwise (out is then "").
int response_get_header(const Response* res, const char* key, char* out, size_t cap);
void response_flush(Response* res, int fd);
void response_cleanup(Response* res);

#endif
