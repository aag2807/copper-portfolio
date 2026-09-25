#ifndef ROUTER_H
#define ROUTER_H

#include "request.h"
#include "response.h"

typedef void (*RouteHandler)(Request*, Response*, void*);

typedef struct
{
    char* method;
    char* pattern; // e.g., "/users/{id}"
    RouteHandler handler;
    void* ctx;
    int has_params; // whether this route has {param} segments
} Route;

typedef struct Router
{
    Route* routes;
    int count;
    int cap;
} Router;

Router* router_create(void);
void router_add(Router* r, const char* method, const char* pattern, RouteHandler handler);
// Finds the handler for req's method + path. GET routes also answer HEAD.
int router_match(Router* r, Request* req, RouteHandler* handler, void** ctx);
// Writes the methods registered for `path` into `out` as an Allow header value
// (e.g. "GET, HEAD"). Returns how many routes matched the path, 0 for none.
int router_allowed(Router* r, const char* path, char* out, size_t cap);
void router_destroy(Router* r);

#endif
