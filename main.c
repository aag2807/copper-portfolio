#include "src/server.h"
#include "src/static.h"

#include <stdio.h>
#include <stdlib.h>

extern void home_index(Request*, Response*, void*);
extern void home_systems(Request*, Response*, void*);
extern void home_gamedev(Request*, Response*, void*);
extern void home_web(Request*, Response*, void*);
extern void home_contact(Request*, Response*, void*);
extern void home_counter(Request*, Response*, void*);
extern void home_todolist(Request*, Response*, void*);

static int logging_middleware(Request* req, Response* res, MiddlewareNode* self, Router* router)
{
    (void)res;
    fprintf(stderr, "[req] %s %s\n", str_cstr(&req->method), str_cstr(&req->path));

    return middleware_next(self, req, res, router);
}

int main(int argc, char* argv[])
{
    int port = 8080;
    if (argc > 1)
        port = atoi(argv[1]);
    printf("\n");
    printf(" ╔══════════════════════════════════════════╗\n");
    printf(" ║       C Copper Web Framework v0.02       ║\n");
    printf(" ╚══════════════════════════════════════════╝\n\n");

    Server* server = server_create(port);

    server_use(server, logging_middleware, NULL);
    server_use(server, static_middleware, NULL);

    // HTML ROUTES
    server_get(server, "/", home_index);
    server_get(server, "/systems", home_systems);
    server_get(server, "/gamedev", home_gamedev);
    server_get(server, "/web", home_web);
    server_get(server, "/contact", home_contact);
    server_get(server, "/counter", home_counter);
    server_get(server, "/todos", home_todolist);

    server_start(server);

    server_destroy(server);

    return 0;
}
