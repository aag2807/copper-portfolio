// Client IP selection, path canonicalisation and template escaping.
#include "../src/request.h"
#include "../src/str.h"
#include "../src/view.h"
#include "test.h"

static void test_xff_last_entry(void)
{
    char ip[64];
    CHECK(request_xff_client_ip("203.0.113.9", ip, sizeof(ip)) == 1);
    CHECK_STR(ip, "203.0.113.9");

    // Client-supplied first entry is ignored; the proxy-appended last one wins.
    CHECK(request_xff_client_ip("1.2.3.4, 10.0.0.1,  198.51.100.7 ", ip, sizeof(ip)) == 1);
    CHECK_STR(ip, "198.51.100.7");

    CHECK(request_xff_client_ip("spoof, 2001:db8::1", ip, sizeof(ip)) == 1);
    CHECK_STR(ip, "2001:db8::1");

    CHECK(request_xff_client_ip("1.2.3.4, ", ip, sizeof(ip)) == 0);
    CHECK(request_xff_client_ip("1.2.3.4, not-an-ip", ip, sizeof(ip)) == 0);
    CHECK(request_xff_client_ip("", ip, sizeof(ip)) == 0);
    CHECK(request_xff_client_ip(NULL, ip, sizeof(ip)) == 0);
}

static void check_canon(const char* in, const char* want, int changed)
{
    String out = str_new();
    CHECK(request_canonical_path(in, &out) == changed);
    CHECK_STR(str_cstr(&out), want);
    str_free(&out);
}

static void test_canonical_path(void)
{
    check_canon("/", "/", 0);
    check_canon("/work", "/work", 0);
    check_canon("/work/", "/work", 1);
    check_canon("//work//systems", "/work/systems", 1);
    check_canon("//", "/", 1);
    check_canon("//evil.example", "/evil.example", 1);
}

static void test_html_escape(void)
{
    String s = str_new();
    str_append_html(&s, "<a href=\"x\">Tom & Jerry's</a>");
    CHECK_STR(str_cstr(&s), "&lt;a href=&quot;x&quot;&gt;Tom &amp; Jerry&#39;s&lt;/a&gt;");
    str_free(&s);

    s = str_new();
    str_append_html(&s, "plain");
    CHECK_STR(str_cstr(&s), "plain");
    str_free(&s);
}

static void test_template_escaping(void)
{
    ViewData data[] = {
        {"path", "text", "/x\"><script>", NULL},
        {"html", "text", "<b>bold</b>", NULL},
        {NULL, NULL, NULL, NULL},
    };

    String out = view_render_string("<link href=\"{{path}}\">", data, NULL);
    CHECK_STR(str_cstr(&out), "<link href=\"/x&quot;&gt;&lt;script&gt;\">");
    str_free(&out);

    out = view_render_string("{{ html }}|{{{html}}}|{{{ html }}}", data, NULL);
    CHECK_STR(str_cstr(&out), "&lt;b&gt;bold&lt;/b&gt;|<b>bold</b>|<b>bold</b>");
    str_free(&out);

    // {{body}} stays raw so the layout can wrap the rendered page.
    out = view_render_string("<main>{{body}}</main>", data, "<p>page</p>");
    CHECK_STR(str_cstr(&out), "<main><p>page</p></main>");
    str_free(&out);
}

int main(void)
{
    printf("test_server\n");
    test_xff_last_entry();
    test_canonical_path();
    test_html_escape();
    test_template_escaping();
    TEST_DONE();
}
