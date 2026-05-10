#include "router.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Router* router_create(void)
{
    Router* r = malloc(sizeof(Router));
    r->routes = NULL;
    r->count = 0;
    r->cap = 0;

    return r;
}

void router_add(Router* r, const char* method, const char* pattern, RouteHandler handler)
{
    if (r->count >= r->cap)
    {
        r->cap = r->cap ? r->cap * 2 : 16;
        r->routes = realloc(r->routes, r->cap * sizeof(Route));
    }

    Route* route = &r->routes[r->count++];
    route->method = strdup(method);
    route->pattern = strdup(pattern);
    route->handler = handler;
    route->ctx = NULL;
    // Check if pattern has {param} segments
    route->has_params = (strchr(pattern, '{') != NULL);
}

// Thread-safe URL pattern matching using strtok_r
static int match_pattern(const char* pattern, const char* path, Request* req)
{
    if (!strchr(pattern, '{'))
    {
        return strcmp(pattern, path) == 0;
    }

    char pat_copy[256], path_copy[256];
    pat_copy[0] = '\0';
    path_copy[0] = '\0';
    strncat(pat_copy, pattern, sizeof(pat_copy) - 1);
    strncat(path_copy, path, sizeof(path_copy) - 1);
    char *save1, *save2;
    char* p_seg = strtok_r(pat_copy, "/", &save1);
    char* path_seg = strtok_r(path_copy, "/", &save2);

    while (p_seg && path_seg)
    {
        if (p_seg[0] == '{')
        {
            char param[64] = {0};
            int i = 1;
            while (p_seg[i] && p_seg[i] != '}' && i < (int)sizeof(param))
            {
                i++;
            }
            request_set_param(req, param, path_seg);
        }
        else if (strcmp(p_seg, path_seg) != 0)
        {
            return 0;
        }

        p_seg = strtok_r(NULL, "/", &save1);
        path_seg = strtok_r(NULL, "/", &save2);
    }
    return (p_seg == NULL && path_seg == NULL);
}

int router_match(Router* r, Request* req, RouteHandler* handler, void** ctx)
{
    for (int i = 0; i < r->count; i++)
    {
        Route* route = &r->routes[i];
        if (strcmp(route->method, str_cstr(&req->method)) != 0)
        {
            continue;
        }

        if (match_pattern(route->pattern, str_cstr(&req->path), req))
        {
            *handler = route->handler;
            *ctx = route->ctx;

            return 1;
        }
    }
    return 0;
}

void router_destroy(Router* r)
{
    for (int i = 0; i < r->count; i++)
    {
        free(r->routes[i].method);
        free(r->routes[i].pattern);
    }

    free(r->routes);
    free(r);
}
