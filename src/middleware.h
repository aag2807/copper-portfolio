#ifndef MIDDLEWARE_H
#define MIDDLEWARE_H

#include "request.h"
#include "response.h"
typedef struct Router Router;
typedef struct MiddlewareNode MiddlewareNode;

typedef int (*MiddlewareFunc)(Request*, Response*, MiddlewareNode* self, Router* router);

struct MiddlewareNode
{
    MiddlewareFunc func;
    void* user_ctx;       // user-provided config context
    MiddlewareNode* next; // next middleware in the chain
};

typedef struct
{
    MiddlewareNode* head;
    MiddlewareNode* tail;
    int count;
} MiddlewarePipeline;

MiddlewarePipeline* middleware_create(void);

void middleware_add(MiddlewarePipeline* p, MiddlewareFunc func, void* ctx);
void middleware_run(MiddlewarePipeline* p, Request* req, Response* res, Router* router);
int middleware_next(MiddlewareNode* self, Request* req, Response* res, Router* router);
void middleware_destroy(MiddlewarePipeline* p);

#endif // !DEBUG
