// Router and dispatch tests: matching, params, HEAD, 405 + Allow, 404.
#include "../src/middleware.h"
#include "../src/request.h"
#include "../src/response.h"
#include "../src/router.h"
#include "test.h"

static int g_hits = 0;
static void h_home(Request* req, Response* res, void* ctx) { (void)req; (void)ctx; g_hits++; response_html(res, "home"); }
static void h_project(Request* req, Response* res, void* ctx) { (void)req; (void)ctx; response_html(res, "project"); }
static void h_contact(Request* req, Response* res, void* ctx) { (void)req; (void)ctx; response_json(res, "{}"); }

static Router* make_router(void)
{
    Router* r = router_create();
    router_add(r, "GET", "/", h_home);
    router_add(r, "GET", "/projects/{slug}", h_project);
    router_add(r, "POST", "/api/contact", h_contact);
    return r;
}

static void req_for(Request* req, const char* method, const char* path)
{
    char raw[512];
    snprintf(raw, sizeof(raw), "%s %s HTTP/1.1\r\nHost: t\r\n\r\n", method, path);
    request_init(req, raw, strlen(raw));
}

static void test_match(void)
{
    Router* r = make_router();
    RouteHandler h;
    void* ctx;
    Request req;

    req_for(&req, "GET", "/");
    CHECK(router_match(r, &req, &h, &ctx) == 1 && h == h_home);
    request_cleanup(&req);

    req_for(&req, "GET", "/projects/c-copper");
    CHECK(router_match(r, &req, &h, &ctx) == 1 && h == h_project);
    CHECK(request_param(&req, "slug") && strcmp(request_param(&req, "slug"), "c-copper") == 0);
    request_cleanup(&req);

    req_for(&req, "GET", "/projects/a/b");
    CHECK(router_match(r, &req, &h, &ctx) == 0);
    request_cleanup(&req);

    req_for(&req, "GET", "/nope");
    CHECK(router_match(r, &req, &h, &ctx) == 0);
    request_cleanup(&req);

    // HEAD is served by the GET route
    req_for(&req, "HEAD", "/");
    CHECK(router_match(r, &req, &h, &ctx) == 1 && h == h_home);
    request_cleanup(&req);

    // POST / is not a route
    req_for(&req, "POST", "/");
    CHECK(router_match(r, &req, &h, &ctx) == 0);
    request_cleanup(&req);

    // HEAD must not reach POST-only routes
    req_for(&req, "HEAD", "/api/contact");
    CHECK(router_match(r, &req, &h, &ctx) == 0);
    request_cleanup(&req);

    router_destroy(r);
}

static void test_allowed(void)
{
    Router* r = make_router();
    char allow[128];
    CHECK(router_allowed(r, "/", allow, sizeof(allow)) == 1);
    CHECK_STR(allow, "GET, HEAD");
    CHECK(router_allowed(r, "/api/contact", allow, sizeof(allow)) == 1);
    CHECK_STR(allow, "POST");
    CHECK(router_allowed(r, "/projects/x", allow, sizeof(allow)) == 1);
    CHECK(router_allowed(r, "/missing", allow, sizeof(allow)) == 0);
    router_destroy(r);
}

static void run(Router* r, MiddlewarePipeline* p, const char* method, const char* path, Response* res)
{
    Request req;
    req_for(&req, method, path);
    response_init(res, -1);
    middleware_run(p, &req, res, r);
    request_cleanup(&req);
}

static void test_dispatch(void)
{
    Router* r = make_router();
    MiddlewarePipeline* p = middleware_create();
    Response res;

    g_hits = 0;
    run(r, p, "HEAD", "/", &res);
    CHECK(res.status_code == 200 && g_hits == 1);
    response_cleanup(&res);

    run(r, p, "POST", "/", &res);
    CHECK(res.status_code == 405);
    CHECK(strstr(str_cstr(&res.headers), "Allow: GET, HEAD\r\n") != NULL);
    response_cleanup(&res);

    run(r, p, "GET", "/api/contact", &res);
    CHECK(res.status_code == 405);
    CHECK(strstr(str_cstr(&res.headers), "Allow: POST\r\n") != NULL);
    response_cleanup(&res);

    run(r, p, "GET", "/does-not-exist", &res);
    CHECK(res.status_code == 404);
    response_cleanup(&res);

    middleware_destroy(p);
    router_destroy(r);
}

int main(void)
{
    printf("test_router\n");
    test_match();
    test_allowed();
    test_dispatch();
    TEST_DONE();
}
