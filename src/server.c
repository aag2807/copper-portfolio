#include "server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define sock_close(fd) closesocket((SOCKET)(fd))
#define sock_read(fd, buf, n) recv((SOCKET)(fd), (buf), (int)(n), 0)
#define poll WSAPoll
#else
#include <netinet/in.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define sock_close(fd) close(fd)
#define sock_read(fd, buf, n) read((fd), (buf), (n))
#endif

// Whole request (headers + body) is capped; headers alone get a smaller cap.
#define REQ_HEADER_MAX (16 * 1024)
#define REQ_BUF_MAX (REQ_HEADER_MAX + REQ_MAX_BODY)
#define SOCKET_TIMEOUT_SEC 10
// Cloud Run allows 10s between SIGTERM and SIGKILL.
#define SHUTDOWN_GRACE_SEC 8

typedef struct
{
    Server* server;
    int client_fd;
    struct sockaddr_in client_addr;
} ClientJob;

static volatile sig_atomic_t g_stop = 0;
static pthread_mutex_t g_active_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_active_cond = PTHREAD_COND_INITIALIZER;
static int g_active = 0;

static void on_stop_signal(int sig)
{
    (void)sig;
    g_stop = 1;
}

static const char* reason_for(int code)
{
    switch (code)
    {
        case 400: return "Bad Request";
        case 408: return "Request Timeout";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 431: return "Request Header Fields Too Large";
        default:  return "Error";
    }
}

static void send_error(int fd, int code)
{
    Response res;
    response_init(&res, fd);
    response_status(&res, code, reason_for(code));
    String body = str_new();
    str_appendf(&body, "<h1>%d - %s</h1>", code, reason_for(code));
    response_html(&res, str_cstr(&body));
    str_free(&body);
    response_flush(&res, fd);
    response_cleanup(&res);
}

// Offset just past the blank line ending the header block, or 0 if not there yet.
static size_t header_end(const char* buf, size_t len)
{
    for (size_t i = 0; i + 1 < len; i++)
    {
        if (buf[i] == '\n' && buf[i + 1] == '\n') return i + 2;
        if (i + 3 < len && buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n') return i + 4;
    }
    return 0;
}

// Waits until fd is readable. Returns 0 on timeout, or straight away when the
// server is shutting down and this connection has not sent anything yet (idle
// keep-alive or pre-opened proxy sockets must not hold up the drain).
static int wait_readable(int fd, int have_bytes)
{
    for (int waited_ms = 0; waited_ms < SOCKET_TIMEOUT_SEC * 1000; waited_ms += 250)
    {
        if (g_stop && !have_bytes) return 0;
        struct pollfd pfd = {.fd = fd, .events = POLLIN, .revents = 0};
        int pr = poll(&pfd, 1, 250);
        if (pr > 0) return 1;
        if (pr < 0 && errno != EINTR) return 0;
    }
    return 0;
}

// Reads one request: until the header block ends and Content-Length body bytes
// have arrived, the peer stops sending, or the size cap is hit. Returns bytes
// read (buf is NUL-terminated) or 0; *status is set to an HTTP error code when
// the request must be rejected before parsing.
static size_t read_request(int fd, char* buf, int* status)
{
    size_t len = 0;
    size_t need = 0; // total bytes expected; 0 until the headers are complete
    *status = 0;

    for (;;)
    {
        if (need && len >= need) break;
        if (len >= REQ_BUF_MAX)
        {
            *status = need ? 413 : 431;
            break;
        }

        if (!wait_readable(fd, len > 0)) break;
        ssize_t n = sock_read(fd, buf + len, REQ_BUF_MAX - len);
        if (n <= 0) break; // EOF, timeout or error: parse what we have
        len += (size_t)n;
        buf[len] = '\0';

        if (!need)
        {
            size_t head = header_end(buf, len);
            if (!head)
            {
                if (len > REQ_HEADER_MAX) { *status = 431; break; }
                continue;
            }

            Request probe;
            int rc = request_init(&probe, buf, head);
            const char* cl = rc ? NULL : request_header(&probe, "Content-Length");
            long body_len = cl ? request_parse_content_length(cl) : 0;
            request_cleanup(&probe);

            if (rc) { *status = rc; break; }
            if (body_len == -1) { *status = 400; break; }
            if (body_len == -2) { *status = 413; break; }
            need = head + (size_t)body_len;
        }
    }
    buf[len] = '\0';
    return len;
}

static void set_client_ip(Request* req, const struct sockaddr_in* addr)
{
    char ip_buf[INET_ADDRSTRLEN];
    const char* peer = inet_ntop(AF_INET, &addr->sin_addr, ip_buf, sizeof(ip_buf));
    if (peer) str_append(&req->client_ip, peer);

    // Behind Cloud Run the peer is the Google front end; it appends the real
    // client address as the LAST X-Forwarded-For entry. Earlier entries are
    // whatever the client sent and must not be trusted.
    char hop[64];
    if (request_xff_client_ip(request_header(req, "X-Forwarded-For"), hop, sizeof(hop)))
    {
        str_free(&req->client_ip);
        req->client_ip = str_from(hop);
    }
}

// 301 non-canonical GET/HEAD paths (//a, /a/) so {{path}} and canonical URLs
// stay clean. Other methods are normalised in place. Returns 1 if it redirected.
static int canonicalize(Request* req, Response* res)
{
    String canon = str_new();
    int changed = request_canonical_path(str_cstr(&req->path), &canon);
    if (!changed)
    {
        str_free(&canon);
        return 0;
    }

    const char* m = str_cstr(&req->method);
    if (strcmp(m, "GET") == 0 || strcmp(m, "HEAD") == 0)
    {
        if (req->query.len)
        {
            str_append(&canon, "?");
            str_append(&canon, str_cstr(&req->query));
        }
        response_redirect_permanent(res, str_cstr(&canon));
        str_free(&canon);
        return 1;
    }

    str_free(&req->path);
    req->path = canon;
    return 0;
}

static void* handle_client(void* arg)
{
    ClientJob* job = (ClientJob*)arg;
    Server* s = job->server;
    int fd = job->client_fd;

    char* buffer = malloc(REQ_BUF_MAX + 1);
    int status = 0;
    size_t bytes_read = buffer ? read_request(fd, buffer, &status) : 0;

    if (status)
    {
        send_error(fd, status);
    }
    else if (bytes_read > 0)
    {
        // Parse into a request
        Request req;
        int rc = request_init(&req, buffer, bytes_read);
        const char* cl = rc ? NULL : request_header(&req, "Content-Length");
        if (!rc && cl && req.body.len < (size_t)request_parse_content_length(cl))
        {
            rc = 400; // body shorter than announced
        }

        if (rc)
        {
            send_error(fd, rc);
        }
        else
        {
            // Stamp the client's IP for downstream middleware (rate limiting, etc.)
            set_client_ip(&req, &job->client_addr);

            // Create a response
            Response res;
            response_init(&res, fd);
            res.omit_body = strcmp(str_cstr(&req.method), "HEAD") == 0;

            // run through middleware pipeline
            if (!canonicalize(&req, &res))
            {
                middleware_run(s->pipeline, &req, &res, s->router);
            }

            // flush responses to socket
            response_flush(&res, fd);
            response_cleanup(&res);
        }
        request_cleanup(&req);
    }

    free(buffer);
    sock_close(fd);
    free(job);

    pthread_mutex_lock(&g_active_lock);
    g_active--;
    pthread_cond_broadcast(&g_active_cond);
    pthread_mutex_unlock(&g_active_lock);

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

    // SIGTERM (Cloud Run / docker stop) and SIGINT stop the accept loop; no
    // SA_RESTART so a blocked poll() returns EINTR straight away.
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_stop_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
#endif

    printf(" ✓ Server listening on http://localhost:%d\n", s->port);

    while (s->running && !g_stop)
    {
        // Poll with a timeout so a signal delivered to a worker thread is
        // still noticed here within half a second.
        struct pollfd pfd = {.fd = fd, .events = POLLIN, .revents = 0};
        int pr = poll(&pfd, 1, 500);
        if (pr <= 0)
        {
            if (pr < 0 && errno != EINTR)
            {
                perror("poll");
                break;
            }
            continue;
        }

        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = (int)accept(fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("accept");
            continue;
        }

        struct timeval tv = {.tv_sec = SOCKET_TIMEOUT_SEC, .tv_usec = 0};
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

        ClientJob* job = malloc(sizeof(ClientJob));
        if (!job)
        {
            sock_close(client_fd);
            continue;
        }
        job->server = s;
        job->client_fd = client_fd;
        job->client_addr = client_addr;

        pthread_mutex_lock(&g_active_lock);
        g_active++;
        pthread_mutex_unlock(&g_active_lock);

        pthread_t thread;
        int rc = pthread_create(&thread, NULL, handle_client, job);
        if (rc != 0)
        {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            pthread_mutex_lock(&g_active_lock);
            g_active--;
            pthread_mutex_unlock(&g_active_lock);
            sock_close(client_fd);
            free(job);
            continue;
        }
        pthread_detach(thread);
    }
    sock_close(fd);
    s->running = 0;

    // Let in-flight requests finish, bounded by the platform's grace period.
    printf(" ✓ Shutting down, draining in-flight requests\n");
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += SHUTDOWN_GRACE_SEC;
    pthread_mutex_lock(&g_active_lock);
    while (g_active > 0)
    {
        if (pthread_cond_timedwait(&g_active_cond, &g_active_lock, &deadline) == ETIMEDOUT) break;
    }
    int left = g_active;
    pthread_mutex_unlock(&g_active_lock);
    if (left > 0) fprintf(stderr, "[server] %d request(s) still running at shutdown\n", left);
#ifdef _WIN32
    WSACleanup();
#endif
}

void server_stop(Server* s)
{
    // The accept loop notices within one poll interval and closes the socket.
    s->running = 0;
    g_stop = 1;
}

void server_destroy(Server* s)
{
    // A worker that outlived the shutdown grace period may still use these.
    pthread_mutex_lock(&g_active_lock);
    int busy = g_active;
    pthread_mutex_unlock(&g_active_lock);
    if (busy > 0) return;

    router_destroy(s->router);
    middleware_destroy(s->pipeline);
    free(s);
}
