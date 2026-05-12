#include "server.h"

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct
{
    Server* server;
    int client_fd;
    struct sockaddr_in client_addr;
} ClientJob;

static void* handle_client(void* arg)
{
    ClientJob* job = (ClientJob*)arg;
    Server* s = job->server;
    int fd = job->client_fd;

    char buffer[65536];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);

    if (bytes_read > 0)
    {
        buffer[bytes_read] = '\0';

        // Parse into a request
        Request req;
        request_init(&req, buffer);

        // Create a response
        Response res;
        response_init(&res, fd);

        // run through middleware pipeline
        middleware_run(s->pipeline, &req, &res, s->router);

        // flush responses to socket
        response_flush(&res, fd);

        request_cleanup(&req);
        response_cleanup(&res);
    }

    close(fd);
    free(job);

    return NULL;
}

Server* server_create(int port)
{
    Server* s = malloc(sizeof(Server));

    s->port = port;
    s->router = router_create();
    s->pipeline = middleware_create();
    s->running = 0;

    return s;
}

void server_use(Server* s, MiddlewareFunc func, void* ctx)
{
    middleware_add(s->pipeline, func, ctx);
}

void server_get(Server* s, const char* path, RouteHandler handler)
{
    router_add(s->router, "GET", path, handler);
}

void server_post(Server* s, const char* path, RouteHandler handler)
{
    router_add(s->router, "POST", path, handler);
}

void server_delete(Server* s, const char* path, RouteHandler handler)
{
    router_add(s->router, "DELETE", path, handler);
}

void server_put(Server* s, const char* path, RouteHandler handler)
{
    router_add(s->router, "PUT", path, handler);
}

void server_patch(Server* s, const char* path, RouteHandler handler)
{
    router_add(s->router, "PATCH", path, handler);
}

void server_start(Server* s)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        perror("socket");
        exit(1);
    }

    int opt = 1;

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(s->port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(fd);
        exit(1);
    }

    if (listen(fd, 128) < 0)
    {
        perror("listen");
        close(fd);
        exit(1);
    }

    s->socked_fd = fd;
    s->running = 1;

    // ignore SIGPIPE so server doesnt die when a client disconnects
    signal(SIGPIPE, SIG_IGN);

    printf(" ✓ Server listening on http://localhost:%d\n", s->port);

    while (s->running)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept(fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0)
        {
            if (errno == EINTR)
            {
                break;
            }
            perror("accept");
            continue;
        }

        ClientJob* job = malloc(sizeof(ClientJob));
        job->server = s;
        job->client_fd = client_fd;
        job->client_addr = client_addr;

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, job);
        pthread_detach(thread);
    }
    close(fd);
}

void server_stop(Server* s)
{
    s->running = 0;
    close(s->socked_fd);
}

void server_destroy(Server* s)
{
    router_destroy(s->router);
    middleware_destroy(s->pipeline);
    free(s);
}
