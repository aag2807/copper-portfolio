
#include "middleware.h"

#include "router.h"
#include "view.h"

#include <stdlib.h>

// Routes the request: handler on match, 405 + Allow when the path exists under
// other methods, otherwise the layout-rendered 404 page.
static void dispatch(Router* router, Request* req, Response* res)
{
    RouteHandler handler;
    void* ctx;
    if (router_match(router, req, &handler, &ctx))
    {
        handler(req, res, ctx);
        return;
    }

    char allow[128];
    if (router_allowed(router, str_cstr(&req->path), allow, sizeof(allow)) > 0)
    {
        response_status(res, 405, "Method Not Allowed");
        response_header(res, "Allow", allow);
        response_html(res, "<h1>405 - Method Not Allowed</h1>");
        return;
    }

    render_not_found(res);
}

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

int middleware_next(MiddlewareNode* self, Request* req, Response* res, Router* router)
{
    if (self->next)
    {
        return self->next->func(req, res, self->next, router);
    }
    // End of chain — route the request
    dispatch(router, req, res);
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
        dispatch(router, req, res);
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
