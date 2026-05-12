#include "src/server.h"
#include "src/static.h"

#include <stdio.h>
#include <stdlib.h>

extern void home_index(Request*, Response*, void*);
extern void home_about(Request*, Response*, void*);
extern void users_list(Request*, Response*, void*);
extern void users_show(Request*, Response*, void*);
extern void api_users(Request*, Response*, void*);

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
    printf(" ╔══════════════════════════════════════╗\n");
    printf(" ║      C MVC Web Framework v0.02        ║\n");
    printf(" ╚══════════════════════════════════════╝\n\n");

    Server* server = server_create(port);

    server_use(server, logging_middleware, NULL);
    server_use(server, static_middleware, NULL);

    server_get(server, "/", home_index);
    server_get(server, "/about", home_about);
    server_get(server, "/users/list", users_list);
    server_get(server, "/users/{id}", users_show);
    server_get(server, "/api/users", api_users);

    server_start(server);

    server_destroy(server);

    return 0;
}
