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

// Parser limits. Anything beyond them is rejected rather than truncated.
#define REQ_MAX_METHOD 16
#define REQ_MAX_TARGET 4096
#define REQ_MAX_VERSION 16
#define REQ_MAX_HEADER_NAME 255
#define REQ_MAX_HEADERS 100
#define REQ_MAX_BODY (64 * 1024)

// Parses `raw_len` bytes of an HTTP/1.x request. Returns 0 on success or the
// HTTP status to answer with (400, 413, 414, 431) when the input is malformed.
// The Request is always initialised, so request_cleanup() is safe either way.
int request_init(Request* req, const char* raw, size_t raw_len);
// Parses a Content-Length value: bytes on success, -1 if malformed, -2 if over REQ_MAX_BODY.
long request_parse_content_length(const char* v);
// Picks the client IP out of an X-Forwarded-For value (the last hop). Returns
// 1 and fills `out` when the entry looks like an IPv4/IPv6 literal, else 0.
int request_xff_client_ip(const char* xff, char* out, size_t cap);
// Writes the canonical form of `path` (no repeated or trailing slashes) into
// `out`, which must be an initialised String. Returns 1 if it differs from path.
int request_canonical_path(const char* path, String* out);
void request_set_param(Request* req, const char* key, const char* value);
const char* request_param(Request* req, const char* key);
const char* request_header(Request* req, const char* key);
void request_cleanup(Request* req);

#endif // !REQUEST_H
