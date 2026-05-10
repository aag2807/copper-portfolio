#ifndef SERVER_H
#define SERVER_H

#include "middleware.h"
#include "request.h"
#include "response.h"
#include "router.h"

typedef struct
{
    int port;
    int socked_fd;
    Router* router;
    MiddlewarePipeline* pipeline;
    int running;
} Server;

Server* server_create(int port);
void server_use(Server* s, MiddlewareFunc func, void* ctx);
void server_get(Server* s, const char* path, RouteHandler handler);
void server_post(Server* s, const char* path, RouteHandler handler);
void server_start(Server* s);
void server_stop(Server* s);
void server_destroy(Server* s);

#endif // !SERVER
