#include "src/csrf.h"
#include "src/email.h"
#include "src/env.h"
#include "src/ratelimit.h"
#include "src/server.h"
#include "src/static.h"
#include "src/telemetry.h"
#include "src/version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void home_index(Request*, Response*, void*);
extern void home_contact(Request*, Response*, void*);
extern void home_workshop(Request*, Response*, void*);
extern void home_counter(Request*, Response*, void*);
extern void home_todolist(Request*, Response*, void*);
extern void home_playground(Request*, Response*, void*);
extern void home_reconcile(Request*, Response*, void*);
extern void contact_submit(Request*, Response*, void*);
extern void home_robots(Request*, Response*, void*);
extern void home_sitemap(Request*, Response*, void*);

extern void work_index(Request*, Response*, void*);
extern void work_fintech_ai(Request*, Response*, void*);
extern void work_systems(Request*, Response*, void*);
extern void work_web(Request*, Response*, void*);
extern void work_gamedev(Request*, Response*, void*);
extern void redirect_systems(Request*, Response*, void*);
extern void redirect_web(Request*, Response*, void*);
extern void redirect_gamedev(Request*, Response*, void*);
extern void redirect_ai(Request*, Response*, void*);

extern void projects_show(Request*, Response*, void*);
extern void writing_index(Request*, Response*, void*);
extern void writing_show(Request*, Response*, void*);
extern void api_status(Request*, Response*, void*);

static int logging_middleware(Request* req, Response* res, MiddlewareNode* self, Router* router)
{
    (void)res;
    fprintf(stderr, "[req] %s %s\n", str_cstr(&req->method), str_cstr(&req->path));

    return middleware_next(self, req, res, router);
}

// $PORT (Cloud Run) wins, then argv[1], then 8080.
static int resolve_port(int argc, char* argv[])
{
    const char* sources[2] = {getenv("PORT"), argc > 1 ? argv[1] : NULL};
    for (int i = 0; i < 2; i++)
    {
        const char* v = sources[i];
        if (!v || !*v) continue;
        char* end = NULL;
        long p = strtol(v, &end, 10);
        if (*end == '\0' && p > 0 && p <= 65535) return (int)p;
        fprintf(stderr, "ignoring invalid port \"%s\"\n", v);
    }
    return 8080;
}

// Prints the startup box with `title` centred in the 42-column interior.
static void print_banner(const char* title)
{
    enum { kInner = 42 };
    int len = (int)strlen(title);
    int left = len < kInner ? (kInner - len + 1) / 2 : 0;
    int right = len < kInner ? kInner - len - left : 0;
    printf(" ╔══════════════════════════════════════════╗\n");
    printf(" ║%*s%s%*s║\n", left, "", title, right, "");
    printf(" ╚══════════════════════════════════════════╝\n\n");
}

int main(int argc, char* argv[])
{
    telemetry_init();

    // Line-buffer stdout so startup logs reach Cloud Run (a pipe) immediately.
    setvbuf(stdout, NULL, _IOLBF, 0);

    int port = resolve_port(argc, argv);
    printf("\n");
    print_banner("C Copper Web Framework v" COPPER_VERSION);

    env_load(".env");
    email_init();
    csrf_init();
    ratelimit_init(5, 3600); // 5 mutating requests per IP per hour

    Server* server = server_create(port);

    server_use(server, logging_middleware, NULL);
    server_use(server, static_middleware, NULL);
    server_use(server, ratelimit_middleware, NULL);
    server_use(server, csrf_middleware, NULL);

    // HTML ROUTES (literals first — router is first-match-wins)
    server_get(server, "/", home_index);
    server_get(server, "/work", work_index);
    server_get(server, "/work/fintech-ai", work_fintech_ai);
    server_get(server, "/work/systems", work_systems);
    server_get(server, "/work/web", work_web);
    server_get(server, "/work/gamedev", work_gamedev);
    server_get(server, "/contact", home_contact);
    server_get(server, "/workshop", home_workshop);
    server_get(server, "/writing", writing_index);
    server_get(server, "/counter", home_counter);
    server_get(server, "/todos", home_todolist);
    server_get(server, "/playground", home_playground);
    server_get(server, "/reconcile", home_reconcile);
    server_get(server, "/robots.txt", home_robots);
    server_get(server, "/sitemap.xml", home_sitemap);

    // 301s from pre-restructure URLs
    server_get(server, "/systems", redirect_systems);
    server_get(server, "/gamedev", redirect_gamedev);
    server_get(server, "/web", redirect_web);
    server_get(server, "/ai", redirect_ai);

    // Param routes last
    server_get(server, "/projects/{slug}", projects_show);
    server_get(server, "/writing/{slug}", writing_show);

    // API ROUTES
    server_get(server, "/api/status", api_status);
    server_post(server, "/api/contact", contact_submit);

    server_start(server);

    server_destroy(server);
    email_cleanup();
    ratelimit_cleanup();

    return 0;
}
