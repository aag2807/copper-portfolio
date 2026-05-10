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

typedef struct
{
    Route* routes;
    int count;
    int cap;
} Router;

Router* router_create(void);
void router_add(Router* r, const char* method, const char* pattern, RouteHandler handler);
int router_match(Router* r, Request* req, RouteHandler* handler, void** ctx);
void router_destroy(Router* r);

#endif
