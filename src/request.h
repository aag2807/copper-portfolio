#ifndef REQUEST_H
#define REQUEST_H

#include "str.h"

#include <stddef.h>

typedef struct
{
    String method;  // GET, POST, PUT, etc.
    String path;    // /users/list
    String query;   // ?page=1&limit=20
    String version; // HTTP/1.1

    struct
    {
        char** keys;
        char** values;
        int count;
        int cap;
    } headers;

    String body;
    String client_ip;

    struct
    {
        char** keys;
        char** values;
        int count;
        int cap;
    } params;

    void* ctx;
} Request;

void request_init(Request* req, const char* raw);
void request_set_param(Request* req, const char* key, const char* value);
const char* request_param(Request* req, const char* key);
const char* request_header(Request* req, const char* key);
void request_cleanup(Request* req);

#endif // !REQUEST_H
