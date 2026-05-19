#include "server.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define sock_close(fd) closesocket((SOCKET)(fd))
#define sock_read(fd, buf, n) recv((SOCKET)(fd), (buf), (int)(n), 0)
#else
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define sock_close(fd) close(fd)
#define sock_read(fd, buf, n) read((fd), (buf), (n))
#endif

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
    ssize_t bytes_read = sock_read(fd, buffer, sizeof(buffer) - 1);

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

    sock_close(fd);
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
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        fprintf(stderr, "WSAStartup failed\n");
        exit(1);
    }
#endif

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        perror("socket");
        exit(1);
    }

    int opt = 1;

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(s->port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        sock_close(fd);
        exit(1);
    }

    if (listen(fd, 128) < 0)
    {
        perror("listen");
        sock_close(fd);
        exit(1);
    }

    s->socked_fd = fd;
    s->running = 1;

#ifndef _WIN32
    // ignore SIGPIPE so server doesnt die when a client disconnects
    signal(SIGPIPE, SIG_IGN);
#endif

    printf(" ✓ Server listening on http://localhost:%d\n", s->port);

    while (s->running)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = (int)accept(fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0)
        {
#ifndef _WIN32
            if (errno == EINTR)
            {
                break;
            }
#endif
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
    sock_close(fd);
#ifdef _WIN32
    WSACleanup();
#endif
}

void server_stop(Server* s)
{
    s->running = 0;
    sock_close(s->socked_fd);
}

void server_destroy(Server* s)
{
    router_destroy(s->router);
    middleware_destroy(s->pipeline);
    free(s);
}
