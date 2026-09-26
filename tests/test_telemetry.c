// Render telemetry: formatting, token substitution, the final response pass
// (Content-Length after substitution) and the /api/status JSON shape.
#include "../src/request.h"
#include "../src/response.h"
#include "../src/str.h"
#include "../src/telemetry.h"
#include "test.h"

#include <stdlib.h>
#include <unistd.h>

static void test_format_count(void)
{
    char buf[40];
    telemetry_format_count(0, buf, sizeof(buf));
    CHECK_STR(buf, "0");
    telemetry_format_count(999, buf, sizeof(buf));
    CHECK_STR(buf, "999");
    telemetry_format_count(1000, buf, sizeof(buf));
    CHECK_STR(buf, "1,000");
    telemetry_format_count(1284, buf, sizeof(buf));
    CHECK_STR(buf, "1,284");
    telemetry_format_count(1234567, buf, sizeof(buf));
    CHECK_STR(buf, "1,234,567");
}

static void test_format_uptime(void)
{
    char buf[32];
    telemetry_format_uptime(0, buf, sizeof(buf));
    CHECK_STR(buf, "0s");
    telemetry_format_uptime(41, buf, sizeof(buf));
    CHECK_STR(buf, "41s");
    telemetry_format_uptime(125, buf, sizeof(buf));
    CHECK_STR(buf, "2m 5s");
    telemetry_format_uptime(3 * 3600 + 12 * 60 + 9, buf, sizeof(buf));
    CHECK_STR(buf, "3h 12m");
    telemetry_format_uptime(2 * 86400 + 4 * 3600 + 59, buf, sizeof(buf));
    CHECK_STR(buf, "2d 4h");
}

static void test_format_ms(void)
{
    char buf[32];
    telemetry_format_ms(0.18, buf, sizeof(buf));
    CHECK_STR(buf, "0.18");
    telemetry_format_ms(12.345, buf, sizeof(buf));
    CHECK(strcmp(buf, "12.35") == 0 || strcmp(buf, "12.34") == 0);
}

static void test_substitute(void)
{
    String s = str_from("<p>no tokens here <!-- plain --></p>");
    char* before = s.data;
    CHECK(telemetry_substitute(&s, "0.18", "1,284", "41s") == 0);
    CHECK(s.data == before); // untouched, no reallocation
    CHECK_STR(str_cstr(&s), "<p>no tokens here <!-- plain --></p>");
    str_free(&s);

    s = str_from("<footer><!--c:render_us-->ms · <!--c:requests--> req · up <!--c:uptime--></footer>");
    CHECK(telemetry_substitute(&s, "0.18", "1,284", "3h 12m") == 3);
    CHECK_STR(str_cstr(&s), "<footer>0.18ms · 1,284 req · up 3h 12m</footer>");
    CHECK(s.len == strlen(s.data));
    str_free(&s);

    // Repeated tokens, an unknown c: comment kept verbatim, token at both ends.
    s = str_from("<!--c:uptime-->|<!--c:uptime-->|<!--c:nope-->|<!--c:render_us-->");
    CHECK(telemetry_substitute(&s, "1.00", "7", "41s") == 3);
    CHECK_STR(str_cstr(&s), "41s|41s|<!--c:nope-->|1.00");
    str_free(&s);

    // A truncated token at the very end must not over-read.
    s = str_from("x<!--c:upt");
    CHECK(telemetry_substitute(&s, "1", "2", "3") == 0);
    CHECK_STR(str_cstr(&s), "x<!--c:upt");
    str_free(&s);
}

static void req_for(Request* req, const char* extra_headers)
{
    char raw[512];
    snprintf(raw, sizeof(raw), "GET / HTTP/1.1\r\nHost: t\r\n%s\r\n", extra_headers);
    request_init(req, raw, strlen(raw));
}

// Flushes `res` into a pipe and returns everything written (caller frees).
static String flush_to_string(Response* res)
{
    int fds[2];
    String out = str_new();
    if (pipe(fds) != 0) return out;
    response_flush(res, fds[1]);
    close(fds[1]);
    char buf[4096];
    ssize_t n;
    while ((n = read(fds[0], buf, sizeof(buf))) > 0) str_append_bytes(&out, buf, (size_t)n);
    close(fds[0]);
    return out;
}

static void test_finish_content_length(void)
{
    telemetry_count_request();
    Request req;
    req_for(&req, "");
    Response res;
    response_init(&res, -1);
    response_html(&res, "<p><!--c:render_us--> / <!--c:requests--> / <!--c:uptime--></p>");

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    telemetry_finish(&req, &res, &start);
    CHECK(strstr(res.body.data, "<!--c:") == NULL);

    char v[64];
    CHECK(response_get_header(&res, "Server-Timing", v, sizeof(v)) == 1);
    CHECK(strncmp(v, "render;dur=", 11) == 0);
    CHECK(response_get_header(&res, "Vary", v, sizeof(v)) == 1);
    CHECK_STR(v, "Accept-Encoding");
    CHECK(response_get_header(&res, "Content-Encoding", v, sizeof(v)) == 0);

    String wire = flush_to_string(&res);
    const char* cl = strstr(wire.data, "Content-Length: ");
    const char* body = strstr(wire.data, "\r\n\r\n");
    CHECK(cl && body);
    if (cl && body)
    {
        size_t want = strtoul(cl + 16, NULL, 10);
        size_t got = wire.len - (size_t)(body + 4 - wire.data);
        CHECK(want == got);
        CHECK(got == res.body.len);
    }
    str_free(&wire);
    response_cleanup(&res);
    request_cleanup(&req);
}

static void test_finish_no_html(void)
{
    // JSON bodies are not scanned for tokens.
    Request req;
    req_for(&req, "");
    Response res;
    response_init(&res, -1);
    response_json(&res, "{\"t\":\"<!--c:uptime-->\"}");
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    telemetry_finish(&req, &res, &start);
    CHECK_STR(str_cstr(&res.body), "{\"t\":\"<!--c:uptime-->\"}");
    response_cleanup(&res);
    request_cleanup(&req);
}

static void test_percentiles(void)
{
    double p50, p99;
    for (int i = 100; i >= 1; i--) telemetry_record((double)i);
    int n = telemetry_percentiles(&p50, &p99);
    CHECK(n >= 100);
    // The two earlier finish() tests added two tiny samples.
    CHECK(p50 >= 48.0 && p50 <= 51.0);
    CHECK(p99 >= 98.0 && p99 <= 100.0);
}

// Finds `"key":` and returns a pointer to the value, or NULL.
static const char* json_value(const char* json, const char* key)
{
    char pat[64];
    snprintf(pat, sizeof(pat), "\"%s\":", key);
    const char* p = strstr(json, pat);
    return p ? p + strlen(pat) : NULL;
}

static void test_status_json(void)
{
    String s = str_new();
    telemetry_status_json(&s);
    const char* j = str_cstr(&s);
    CHECK(j[0] == '{' && j[s.len - 1] == '}');

    const char* v = json_value(j, "server");
    CHECK(v && strncmp(v, "\"c-copper\"", 10) == 0);
    v = json_value(j, "version");
    CHECK(v && v[0] == '"');
    v = json_value(j, "build");
    CHECK(v && v[0] == '"' && v[1] != '"');

    const char* nums[] = {"uptime_s", "requests", "p50", "p99", "samples", "rss_kb", "binary_kb", "pid"};
    for (size_t i = 0; i < sizeof(nums) / sizeof(nums[0]); i++)
    {
        v = json_value(j, nums[i]);
        CHECK(v != NULL);
        if (!v) continue;
        char* end = NULL;
        double d = strtod(v, &end);
        CHECK(end != v && d >= 0);
        CHECK(*end == ',' || *end == '}');
    }
    v = json_value(j, "render_ms");
    CHECK(v && v[0] == '{');
    v = json_value(j, "requests");
    CHECK(v && strtod(v, NULL) >= 1);
    v = json_value(j, "pid");
    CHECK(v && strtod(v, NULL) > 0);

    // Balanced braces and quotes: a cheap well-formedness check.
    int depth = 0, quotes = 0;
    for (const char* p = j; *p; p++)
    {
        if (*p == '"') quotes++;
        else if (*p == '{') depth++;
        else if (*p == '}') depth--;
        CHECK(depth >= 0);
    }
    CHECK(depth == 0 && quotes % 2 == 0);
    str_free(&s);
}

int main(void)
{
    printf("test_telemetry\n");
    telemetry_init();
    test_format_count();
    test_format_uptime();
    test_format_ms();
    test_substitute();
    test_finish_content_length();
    test_finish_no_html();
    test_percentiles();
    test_status_json();
    TEST_DONE();
}
