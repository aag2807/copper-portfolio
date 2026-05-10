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
} Response;

void response_init(Response* res, int fd);
void response_status(Response* res, int code, const char* text);
void response_header(Response* res, const char* key, const char* value);
void response_send(Response* res, const char* body);
void response_json(Response* res, const char* json);
void response_html(Response* res, const char* html);
void response_redirect(Response* res, const char* url);
void response_flush(Response* res, int fd);
void response_cleanup(Response* res);

#endif
