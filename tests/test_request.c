// Request parser tests.
#include "../src/request.h"
#include "test.h"

#include <stdlib.h>

static int parse(const char* raw, Request* req)
{
    return request_init(req, raw, strlen(raw));
}

static void test_valid_get(void)
{
    Request req;
    int rc = parse("GET /work?x=1 HTTP/1.1\r\nHost: example.com\r\nX-Thing:  padded  \r\n\r\n", &req);
    CHECK(rc == 0);
    CHECK_STR(str_cstr(&req.method), "GET");
    CHECK_STR(str_cstr(&req.path), "/work");
    CHECK_STR(str_cstr(&req.query), "x=1");
    CHECK_STR(str_cstr(&req.version), "HTTP/1.1");
    CHECK_STR(request_header(&req, "host"), "example.com");
    CHECK_STR(request_header(&req, "X-Thing"), "padded");
    request_cleanup(&req);
}

static void test_body(void)
{
    Request req;
    int rc = parse("POST /api/contact HTTP/1.1\r\nContent-Length: 5\r\n\r\nhello-extra", &req);
    CHECK(rc == 0);
    CHECK_STR(str_cstr(&req.body), "hello");
    request_cleanup(&req);
}

// Regression for the key[256] stack overflow: a 2000-byte header name.
static void test_oversized_header_name(void)
{
    String raw = str_from("GET / HTTP/1.1\r\n");
    for (int i = 0; i < 2000; i++) str_append(&raw, "A");
    str_append(&raw, ": x\r\n\r\n");
    Request req;
    CHECK(request_init(&req, raw.data, raw.len) == 400);
    request_cleanup(&req);
    str_free(&raw);

    // 255 is the longest accepted name
    raw = str_from("GET / HTTP/1.1\r\n");
    for (int i = 0; i < REQ_MAX_HEADER_NAME; i++) str_append(&raw, "B");
    str_append(&raw, ": ok\r\n\r\n");
    CHECK(request_init(&req, raw.data, raw.len) == 0);
    CHECK(req.headers.count == 1);
    request_cleanup(&req);
    str_free(&raw);
}

static void test_header_without_colon(void)
{
    Request req;
    CHECK(parse("GET / HTTP/1.1\r\nNoColonHere\r\n\r\n", &req) == 400);
    request_cleanup(&req);
    CHECK(parse("GET / HTTP/1.1\r\n: empty-name\r\n\r\n", &req) == 400);
    request_cleanup(&req);
    CHECK(parse("GET / HTTP/1.1\r\nBad Name: v\r\n\r\n", &req) == 400);
    request_cleanup(&req);
}

static void test_long_header_value_kept_whole(void)
{
    String raw = str_from("GET / HTTP/1.1\r\nCookie: ");
    for (int i = 0; i < 3000; i++) str_append(&raw, "c");
    str_append(&raw, "\r\nHost: h\r\n\r\n");
    Request req;
    CHECK(request_init(&req, raw.data, raw.len) == 0);
    CHECK(strlen(request_header(&req, "Cookie")) == 3000);
    CHECK_STR(request_header(&req, "Host"), "h");
    CHECK(req.headers.count == 2);
    request_cleanup(&req);
    str_free(&raw);
}

static void test_bad_request_line(void)
{
    Request req;
    CHECK(parse("GET /a\rb HTTP/1.1\r\n\r\n", &req) == 400);
    request_cleanup(&req);
    CHECK(parse("GET relative HTTP/1.1\r\n\r\n", &req) == 400);
    request_cleanup(&req);
    CHECK(parse("get / HTTP/1.1\r\n\r\n", &req) == 400);
    request_cleanup(&req);
    CHECK(parse("GET / FTP/1.0\r\n\r\n", &req) == 400);
    request_cleanup(&req);
    CHECK(parse("", &req) == 400);
    request_cleanup(&req);

    String raw = str_from("GET /");
    for (int i = 0; i < REQ_MAX_TARGET + 10; i++) str_append(&raw, "p");
    str_append(&raw, " HTTP/1.1\r\n\r\n");
    CHECK(request_init(&req, raw.data, raw.len) == 414);
    request_cleanup(&req);
    str_free(&raw);
}

static void test_content_length(void)
{
    CHECK(request_parse_content_length("0") == 0);
    CHECK(request_parse_content_length("42") == 42);
    CHECK(request_parse_content_length("") == -1);
    CHECK(request_parse_content_length("-1") == -1);
    CHECK(request_parse_content_length("12abc") == -1);
    CHECK(request_parse_content_length("99999999999999999999") == -2);

    Request req;
    CHECK(parse("POST / HTTP/1.1\r\nContent-Length: nope\r\n\r\n", &req) == 400);
    request_cleanup(&req);
    CHECK(parse("POST / HTTP/1.1\r\nContent-Length: 10000000\r\n\r\n", &req) == 413);
    request_cleanup(&req);
}

static void test_too_many_headers(void)
{
    String raw = str_from("GET / HTTP/1.1\r\n");
    for (int i = 0; i < REQ_MAX_HEADERS + 1; i++) str_appendf(&raw, "H%d: v\r\n", i);
    str_append(&raw, "\r\n");
    Request req;
    CHECK(request_init(&req, raw.data, raw.len) == 431);
    request_cleanup(&req);
    str_free(&raw);
}

int main(void)
{
    printf("test_request\n");
    test_valid_get();
    test_body();
    test_oversized_header_name();
    test_header_without_colon();
    test_long_header_value_kept_whole();
    test_bad_request_line();
    test_content_length();
    test_too_many_headers();
    TEST_DONE();
}
