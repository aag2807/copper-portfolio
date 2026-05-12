
#include "middleware.h"

#include "router.h"

#include <stdio.h>
#include <stdlib.h>
MiddlewarePipeline* middleware_create(void)
{
    return calloc(1, sizeof(MiddlewarePipeline));
}
void middleware_add(MiddlewarePipeline* p, MiddlewareFunc func, void* ctx)
{
    MiddlewareNode* node = malloc(sizeof(MiddlewareNode));
    node->func = func;
    node->user_ctx = ctx;
    node->next = NULL;
    if (p->tail)
    {
        p->tail->next = node;
    }
    else
    {
        p->head = node;
    }
    p->tail = node;
    p->count++;
}
// Call the next middleware in the chain,
// or route the request if at the end
int middleware_next(MiddlewareNode* self, Request* req, Response* res, Router* router)
{
    if (self->next)
    {
        return self->next->func(req, res, self->next, router);
    }
    // End of chain — route the request
    RouteHandler handler;
    void* ctx;
    if (router_match(router, req, &handler, &ctx))
    {
        handler(req, res, ctx);
    }
    else
    {
        response_status(res, 404, "Not Found");
        response_html(res, "<h1>404 - Page Not Found</h1>");
    }
    return 1;
}
void middleware_run(MiddlewarePipeline* p, Request* req, Response* res, Router* router)
{
    if (p->head)
    {
        p->head->func(req, res, p->head, router);
    }
    else
    {
        // No middleware — route directly
        RouteHandler handler;
        void* ctx;
        if (router_match(router, req, &handler, &ctx))
        {
            handler(req, res, ctx);
        }
        else
        {
            response_status(res, 404, "Not Found");
            response_html(res, "<h1>404 - Page Not Found</h1>");
        }
    }
}
void middleware_destroy(MiddlewarePipeline* p)
{
    MiddlewareNode* cur = p->head;
    while (cur)
    {
        MiddlewareNode* next = cur->next;
        free(cur);
        cur = next;
    }
    free(p);
}
