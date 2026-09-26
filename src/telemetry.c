#define _GNU_SOURCE // memmem
#include "telemetry.h"

#include "gzip.h"
#include "version.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static atomic_ullong g_requests = 0;
static struct timespec g_start;
static pthread_once_t g_start_once = PTHREAD_ONCE_INIT;

static pthread_mutex_t g_ring_lock = PTHREAD_MUTEX_INITIALIZER;
static double g_ring[TELEMETRY_RING];
static int g_ring_next = 0;
static int g_ring_count = 0;

static void record_start(void)
{
    clock_gettime(CLOCK_MONOTONIC, &g_start);
}

void telemetry_init(void)
{
    pthread_once(&g_start_once, record_start);
}

unsigned long long telemetry_count_request(void)
{
    return atomic_fetch_add_explicit(&g_requests, 1, memory_order_relaxed) + 1;
}

unsigned long long telemetry_requests(void)
{
    return atomic_load_explicit(&g_requests, memory_order_relaxed);
}

long telemetry_uptime_s(void)
{
    telemetry_init();
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (long)(now.tv_sec - g_start.tv_sec);
}

void telemetry_record(double ms)
{
    pthread_mutex_lock(&g_ring_lock);
    g_ring[g_ring_next] = ms;
    g_ring_next = (g_ring_next + 1) % TELEMETRY_RING;
    if (g_ring_count < TELEMETRY_RING) g_ring_count++;
    pthread_mutex_unlock(&g_ring_lock);
}

static int cmp_double(const void* a, const void* b)
{
    double x = *(const double*)a, y = *(const double*)b;
    return (x > y) - (x < y);
}

// Nearest-rank percentile over a sorted array.
static double rank(const double* sorted, int n, double p)
{
    int idx = (int)(p * n + 0.999999) - 1;
    if (idx < 0) idx = 0;
    if (idx >= n) idx = n - 1;
    return sorted[idx];
}

int telemetry_percentiles(double* p50, double* p99)
{
    double copy[TELEMETRY_RING];
    pthread_mutex_lock(&g_ring_lock);
    int n = g_ring_count;
    memcpy(copy, g_ring, sizeof(double) * (size_t)n);
    pthread_mutex_unlock(&g_ring_lock);

    *p50 = *p99 = 0.0;
    if (n == 0) return 0;
    qsort(copy, (size_t)n, sizeof(double), cmp_double);
    *p50 = rank(copy, n, 0.50);
    *p99 = rank(copy, n, 0.99);
    return n;
}

void telemetry_format_ms(double ms, char* out, size_t cap)
{
    snprintf(out, cap, "%.2f", ms < 0 ? 0.0 : ms);
}

void telemetry_format_count(unsigned long long n, char* out, size_t cap)
{
    char digits[32];
    int len = snprintf(digits, sizeof(digits), "%llu", n);
    size_t o = 0;
    for (int i = 0; i < len && o + 1 < cap; i++)
    {
        if (i > 0 && (len - i) % 3 == 0)
        {
            out[o++] = ',';
            if (o + 1 >= cap) break;
        }
        out[o++] = digits[i];
    }
    if (cap) out[o < cap ? o : cap - 1] = '\0';
}

void telemetry_format_uptime(long s, char* out, size_t cap)
{
    if (s < 0) s = 0;
    if (s < 60)
        snprintf(out, cap, "%lds", s);
    else if (s < 3600)
        snprintf(out, cap, "%ldm %lds", s / 60, s % 60);
    else if (s < 86400)
        snprintf(out, cap, "%ldh %ldm", s / 3600, (s % 3600) / 60);
    else
        snprintf(out, cap, "%ldd %ldh", s / 86400, (s % 86400) / 3600);
}

int telemetry_substitute(String* body, const char* render_ms, const char* requests, const char* uptime)
{
    static const char kPrefix[] = "<!--c:";
    if (!body->data || !memmem(body->data, body->len, kPrefix, sizeof(kPrefix) - 1)) return 0;

    const struct
    {
        const char* token;
        size_t len;
        const char* value;
    } subs[] = {
        {TELEMETRY_TOKEN_RENDER, sizeof(TELEMETRY_TOKEN_RENDER) - 1, render_ms},
        {TELEMETRY_TOKEN_REQUESTS, sizeof(TELEMETRY_TOKEN_REQUESTS) - 1, requests},
        {TELEMETRY_TOKEN_UPTIME, sizeof(TELEMETRY_TOKEN_UPTIME) - 1, uptime},
    };

    String out = str_new();
    int replaced = 0;
    const char* p = body->data;
    const char* end = body->data + body->len;
    while (p < end)
    {
        const char* hit = memmem(p, (size_t)(end - p), kPrefix, sizeof(kPrefix) - 1);
        if (!hit)
        {
            str_append_bytes(&out, p, (size_t)(end - p));
            break;
        }
        str_append_bytes(&out, p, (size_t)(hit - p));

        size_t i;
        for (i = 0; i < sizeof(subs) / sizeof(subs[0]); i++)
        {
            if ((size_t)(end - hit) >= subs[i].len && memcmp(hit, subs[i].token, subs[i].len) == 0) break;
        }
        if (i < sizeof(subs) / sizeof(subs[0]))
        {
            str_append(&out, subs[i].value);
            p = hit + subs[i].len;
            replaced++;
        }
        else
        {
            // Some other comment that shares the prefix: keep it verbatim.
            str_append_bytes(&out, hit, sizeof(kPrefix) - 1);
            p = hit + sizeof(kPrefix) - 1;
        }
    }

    if (replaced)
    {
        str_free(body);
        *body = out;
    }
    else
    {
        str_free(&out);
    }
    return replaced;
}

// VmRSS from /proc/self/status, in kB; 0 when unavailable (non-Linux).
static long rss_kb(void)
{
    FILE* f = fopen("/proc/self/status", "r");
    if (!f) return 0;
    char line[256];
    long kb = 0;
    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            kb = strtol(line + 6, NULL, 10);
            break;
        }
    }
    fclose(f);
    return kb < 0 ? 0 : kb;
}

static long binary_kb(void)
{
    struct stat st;
    if (stat("/proc/self/exe", &st) != 0) return 0;
    return (long)(st.st_size / 1024);
}

void telemetry_status_json(String* out)
{
    // BUILD_SHA comes from the build environment; keep only safe characters so
    // it can never break out of the JSON string.
    char build[64];
    size_t b = 0;
    for (const char* s = BUILD_SHA; *s && b + 1 < sizeof(build); s++)
    {
        char c = *s;
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' || c == '-' || c == '_')
            build[b++] = c;
    }
    build[b] = '\0';

    double p50, p99;
    int samples = telemetry_percentiles(&p50, &p99);
    str_appendf(out,
                "{\"server\":\"%s\",\"version\":\"%s\",\"build\":\"%s\",\"uptime_s\":%ld,\"requests\":%llu,"
                "\"render_ms\":{\"p50\":%.3f,\"p99\":%.3f,\"samples\":%d},"
                "\"rss_kb\":%ld,\"binary_kb\":%ld,\"pid\":%ld}",
                COPPER_NAME, COPPER_VERSION, b ? build : "dev", telemetry_uptime_s(), telemetry_requests(), p50, p99,
                samples, rss_kb(), binary_kb(), (long)getpid());
}

static int is_html(const char* ct)
{
    return strncmp(ct, "text/html", 9) == 0;
}

void telemetry_finish(Request* req, Response* res, const struct timespec* start)
{
    // The render time necessarily stops here, just before the tokens are
    // filled in and the body is compressed; those steps are not included.
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double ms = (double)(now.tv_sec - start->tv_sec) * 1e3 + (double)(now.tv_nsec - start->tv_nsec) / 1e6;
    if (ms < 0) ms = 0;

    char ctype[128];
    response_get_header(res, "Content-Type", ctype, sizeof(ctype));

    // Encoded or ETag'd bodies (static files) must stay byte-exact.
    if (is_html(ctype) && !response_get_header(res, "Content-Encoding", NULL, 0) &&
        !response_get_header(res, "ETag", NULL, 0))
    {
        char render[32], count[40], uptime[32];
        telemetry_format_ms(ms, render, sizeof(render));
        telemetry_format_count(telemetry_requests(), count, sizeof(count));
        telemetry_format_uptime(telemetry_uptime_s(), uptime, sizeof(uptime));
        telemetry_substitute(&res->body, render, count, uptime);
    }

    telemetry_record(ms);

    char timing[64];
    snprintf(timing, sizeof(timing), "render;dur=%.3f", ms);
    response_header(res, "Server-Timing", timing);

    if (!gzip_compressible(ctype)) return;
    if (!response_get_header(res, "Vary", NULL, 0)) response_header(res, "Vary", "Accept-Encoding");

    // Already encoded (static cache), empty-bodied status, or too small.
    if (res->status_code == 204 || res->status_code == 304) return;
    if (res->body.len <= GZIP_MIN_BYTES) return;
    if (response_get_header(res, "Content-Encoding", NULL, 0)) return;
    if (!req || !gzip_accepted(request_header(req, "Accept-Encoding"))) return;

    // HEAD still builds the body, so its Content-Length matches the GET's.
    String gz = str_new();
    if (gzip_compress(res->body.data, res->body.len, &gz) != 0)
    {
        str_free(&gz);
        return;
    }
    str_free(&res->body);
    res->body = gz;
    response_header(res, "Content-Encoding", "gzip");
}
