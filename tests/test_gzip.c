// gzip: Accept-Encoding negotiation, compressible types, round trips through
// zlib, the response pass, and the static-file gzip variant + ETag + 304.
#include "../src/gzip.h"
#include "../src/request.h"
#include "../src/response.h"
#include "../src/static.h"
#include "../src/str.h"
#include "../src/telemetry.h"
#include "test.h"

#include <stdlib.h>
#include <unistd.h>
#include <zlib.h>

// Inflates a gzip stream; returns 0 on success.
static int gunzip(const void* data, size_t len, String* out)
{
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    if (inflateInit2(&zs, 15 + 16) != Z_OK) return -1;
    zs.next_in = (Bytef*)data;
    zs.avail_in = (uInt)len;
    unsigned char buf[4096];
    int rc;
    do
    {
        zs.next_out = buf;
        zs.avail_out = sizeof(buf);
        rc = inflate(&zs, Z_NO_FLUSH);
        if (rc != Z_OK && rc != Z_STREAM_END) break;
        str_append_bytes(out, buf, sizeof(buf) - zs.avail_out);
    } while (rc != Z_STREAM_END);
    inflateEnd(&zs);
    return rc == Z_STREAM_END ? 0 : -1;
}

static void test_accept_encoding(void)
{
    CHECK(gzip_accepted("gzip") == 1);
    CHECK(gzip_accepted("gzip, deflate, br") == 1);
    CHECK(gzip_accepted("br;q=1.0, gzip;q=0.8, *;q=0.1") == 1);
    CHECK(gzip_accepted("GZIP") == 1);
    CHECK(gzip_accepted("x-gzip") == 1);
    CHECK(gzip_accepted("gzip;q=0") == 0);
    CHECK(gzip_accepted("gzip; q=0.000") == 0);
    CHECK(gzip_accepted("gzip;q=0, *") == 0);  // explicit entry beats *
    CHECK(gzip_accepted("br") == 0);
    CHECK(gzip_accepted("br, deflate") == 0);
    CHECK(gzip_accepted("*") == 1);
    CHECK(gzip_accepted("*;q=0") == 0);
    CHECK(gzip_accepted("identity") == 0);
    CHECK(gzip_accepted("gzipper") == 0);
    CHECK(gzip_accepted("") == 0);
    CHECK(gzip_accepted(NULL) == 0);
}

static void test_compressible(void)
{
    CHECK(gzip_compressible("text/html; charset=utf-8") == 1);
    CHECK(gzip_compressible("text/css; charset=utf-8") == 1);
    CHECK(gzip_compressible("text/plain") == 1);
    CHECK(gzip_compressible("application/javascript; charset=utf-8") == 1);
    CHECK(gzip_compressible("application/json") == 1);
    CHECK(gzip_compressible("application/xml; charset=utf-8") == 1);
    CHECK(gzip_compressible("image/svg+xml") == 1);
    CHECK(gzip_compressible("application/wasm") == 1);
    CHECK(gzip_compressible("image/png") == 0);
    CHECK(gzip_compressible("font/woff2") == 0);
    CHECK(gzip_compressible("application/pdf") == 0);
    CHECK(gzip_compressible("application/jsonx") == 0);
    CHECK(gzip_compressible("") == 0);
    CHECK(gzip_compressible(NULL) == 0);
}

static void make_text(String* s, size_t approx)
{
    int i = 0;
    while (s->len < approx) str_appendf(s, "<li class=\"row-%d\">line %d of the round trip</li>\n", i % 7, i), i++;
}

static void test_round_trip(void)
{
    String src = str_new();
    make_text(&src, 50000);
    String gz = str_new();
    CHECK(gzip_compress(src.data, src.len, &gz) == 0);
    CHECK(gz.len > 18 && gz.len < src.len / 4);
    CHECK((unsigned char)gz.data[0] == 0x1f && (unsigned char)gz.data[1] == 0x8b); // gzip magic
    String back = str_new();
    CHECK(gunzip(gz.data, gz.len, &back) == 0);
    CHECK(back.len == src.len && memcmp(back.data, src.data, src.len) == 0);
    str_free(&src);
    str_free(&gz);
    str_free(&back);

    // Binary bytes including NULs survive too.
    unsigned char bin[3000];
    for (size_t k = 0; k < sizeof(bin); k++) bin[k] = (unsigned char)(k * 31 + (k >> 3));
    gz = str_new();
    back = str_new();
    CHECK(gzip_compress(bin, sizeof(bin), &gz) == 0);
    CHECK(gunzip(gz.data, gz.len, &back) == 0);
    CHECK(back.len == sizeof(bin) && memcmp(back.data, bin, sizeof(bin)) == 0);
    str_free(&gz);
    str_free(&back);
}

static void req_for(Request* req, const char* method, const char* headers)
{
    char raw[512];
    snprintf(raw, sizeof(raw), "%s / HTTP/1.1\r\nHost: t\r\n%s\r\n", method, headers);
    request_init(req, raw, strlen(raw));
}

static void test_finish_gzip(void)
{
    String page = str_new();
    make_text(&page, 8000);

    // Negotiated: body replaced by gzip bytes that inflate to the original.
    Request req;
    req_for(&req, "GET", "Accept-Encoding: gzip, br\r\n");
    Response res;
    response_init(&res, -1);
    response_html(&res, page.data);
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    telemetry_finish(&req, &res, &start);
    char v[64];
    CHECK(response_get_header(&res, "Content-Encoding", v, sizeof(v)) == 1);
    CHECK_STR(v, "gzip");
    CHECK(response_get_header(&res, "Vary", v, sizeof(v)) == 1);
    CHECK(res.body.len < page.len);
    String back = str_new();
    CHECK(gunzip(res.body.data, res.body.len, &back) == 0);
    CHECK(back.len == page.len && memcmp(back.data, page.data, page.len) == 0);
    str_free(&back);
    response_cleanup(&res);
    request_cleanup(&req);

    // Not negotiated: identity body, still Vary.
    req_for(&req, "GET", "Accept-Encoding: gzip;q=0\r\n");
    response_init(&res, -1);
    response_html(&res, page.data);
    telemetry_finish(&req, &res, &start);
    CHECK(response_get_header(&res, "Content-Encoding", NULL, 0) == 0);
    CHECK(response_get_header(&res, "Vary", NULL, 0) == 1);
    CHECK(res.body.len == page.len);
    response_cleanup(&res);
    request_cleanup(&req);

    // Small bodies and non-text types stay identity; images get no Vary.
    req_for(&req, "GET", "Accept-Encoding: gzip\r\n");
    response_init(&res, -1);
    response_html(&res, "<p>tiny</p>");
    telemetry_finish(&req, &res, &start);
    CHECK(response_get_header(&res, "Content-Encoding", NULL, 0) == 0);
    response_cleanup(&res);
    response_init(&res, -1);
    response_bytes(&res, "image/png", page.data, page.len);
    telemetry_finish(&req, &res, &start);
    CHECK(response_get_header(&res, "Content-Encoding", NULL, 0) == 0);
    CHECK(response_get_header(&res, "Vary", NULL, 0) == 0);
    response_cleanup(&res);
    request_cleanup(&req);

    str_free(&page);
}

static void test_static_variant(void)
{
    const char* dir = getenv("TMPDIR");
    char path[512];
    snprintf(path, sizeof(path), "%s/copper-gz-XXXXXX", dir && *dir ? dir : "/tmp");
    int fd = mkstemp(path);
    CHECK(fd >= 0);
    if (fd < 0) return;
    String css = str_new();
    while (css.len < 20000) str_append(&css, ".card { color: var(--ink); margin: 0 auto; }\n");
    CHECK(write(fd, css.data, css.len) == (ssize_t)css.len);
    close(fd);

    const char* ct = "text/css; charset=utf-8";
    char etag_plain[64] = "", etag_gz[64] = "", v[64];

    // Identity.
    Request req;
    Response res;
    req_for(&req, "GET", "");
    response_init(&res, -1);
    CHECK(static_send_file(&req, &res, path, ct) == 1);
    CHECK(response_get_header(&res, "ETag", etag_plain, sizeof(etag_plain)) == 1);
    CHECK(response_get_header(&res, "Content-Encoding", NULL, 0) == 0);
    CHECK(response_get_header(&res, "Vary", NULL, 0) == 1);
    CHECK(res.body.len == css.len);
    response_cleanup(&res);
    request_cleanup(&req);

    // gzip twice: first compresses, second comes from the cache; same bytes.
    String first = str_new();
    for (int round = 0; round < 2; round++)
    {
        req_for(&req, "GET", "Accept-Encoding: gzip\r\n");
        response_init(&res, -1);
        CHECK(static_send_file(&req, &res, path, ct) == 1);
        CHECK(response_get_header(&res, "ETag", etag_gz, sizeof(etag_gz)) == 1);
        CHECK(response_get_header(&res, "Content-Encoding", v, sizeof(v)) == 1);
        CHECK_STR(v, "gzip");
        if (round == 0)
            str_append_bytes(&first, res.body.data, res.body.len);
        else
            CHECK(res.body.len == first.len && memcmp(res.body.data, first.data, first.len) == 0);
        String back = str_new();
        CHECK(gunzip(res.body.data, res.body.len, &back) == 0);
        CHECK(back.len == css.len && memcmp(back.data, css.data, css.len) == 0);
        str_free(&back);
        response_cleanup(&res);
        request_cleanup(&req);
    }
    str_free(&first);

    // The gzip ETag is the plain one with -gz inside the quotes.
    size_t pl = strlen(etag_plain);
    CHECK(pl > 2 && strncmp(etag_gz, etag_plain, pl - 1) == 0 && strcmp(etag_gz + pl - 1, "-gz\"") == 0);

    // If-None-Match only matches the variant being served.
    char hdr[256];
    snprintf(hdr, sizeof(hdr), "Accept-Encoding: gzip\r\nIf-None-Match: %s\r\n", etag_gz);
    req_for(&req, "GET", hdr);
    response_init(&res, -1);
    static_send_file(&req, &res, path, ct);
    CHECK(res.status_code == 304);
    CHECK(response_get_header(&res, "ETag", v, sizeof(v)) == 1 && strcmp(v, etag_gz) == 0);
    response_cleanup(&res);
    request_cleanup(&req);

    snprintf(hdr, sizeof(hdr), "Accept-Encoding: gzip\r\nIf-None-Match: %s\r\n", etag_plain);
    req_for(&req, "GET", hdr);
    response_init(&res, -1);
    static_send_file(&req, &res, path, ct);
    CHECK(res.status_code == 200);
    response_cleanup(&res);
    request_cleanup(&req);

    snprintf(hdr, sizeof(hdr), "If-None-Match: %s\r\n", etag_gz);
    req_for(&req, "GET", hdr);
    response_init(&res, -1);
    static_send_file(&req, &res, path, ct);
    CHECK(res.status_code == 200);
    response_cleanup(&res);
    request_cleanup(&req);

    snprintf(hdr, sizeof(hdr), "If-None-Match: %s\r\n", etag_plain);
    req_for(&req, "GET", hdr);
    response_init(&res, -1);
    static_send_file(&req, &res, path, ct);
    CHECK(res.status_code == 304);
    response_cleanup(&res);
    request_cleanup(&req);

    // HEAD reports the gzip length (the body is built, just not written).
    req_for(&req, "HEAD", "Accept-Encoding: gzip\r\n");
    response_init(&res, -1);
    res.omit_body = 1;
    static_send_file(&req, &res, path, ct);
    CHECK(response_get_header(&res, "Content-Encoding", NULL, 0) == 1);
    CHECK(res.body.len > 0 && res.body.len < css.len);
    response_cleanup(&res);
    request_cleanup(&req);

    unlink(path);
    str_free(&css);
}

int main(void)
{
    printf("test_gzip\n");
    test_accept_encoding();
    test_compressible();
    test_round_trip();
    test_finish_gzip();
    test_static_variant();
    TEST_DONE();
}
