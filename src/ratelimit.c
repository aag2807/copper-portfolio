#include "ratelimit.h"
#include "response.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUCKETS 256
// Hard cap on tracked IPs. Spoofed or rotating addresses cannot grow memory
// past this; when full and nothing has expired, new IPs are refused (429).
#define MAX_ENTRIES 10000

typedef struct Entry
{
    char ip[64];
    int count;
    long window_start;
    struct Entry* next;
} Entry;

static Entry* g_buckets[BUCKETS];
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_max = 5;
static int g_window = 3600;
static int g_entries = 0;

void ratelimit_init(int max_per_window, int window_seconds)
{
    if (max_per_window > 0)    g_max = max_per_window;
    if (window_seconds > 0)    g_window = window_seconds;
    memset(g_buckets, 0, sizeof(g_buckets));
}

static unsigned int hash_ip(const char* s)
{
    unsigned int h = 2166136261u;
    for (; *s; s++) { h ^= (unsigned char)*s; h *= 16777619u; }
    return h % BUCKETS;
}

// Drops every expired entry in bucket b. Caller holds g_lock.
static void evict_bucket(unsigned int b, long now, const char* keep)
{
    Entry** link = &g_buckets[b];
    while (*link)
    {
        Entry* e = *link;
        if (now - e->window_start >= g_window && (!keep || strcmp(e->ip, keep) != 0))
        {
            *link = e->next;
            free(e);
            g_entries--;
            continue;
        }
        link = &e->next;
    }
}

// Returns the entry for ip, or NULL if the table is full. Caller holds g_lock.
static Entry* lookup_or_create(const char* ip, long now)
{
    unsigned int b = hash_ip(ip);
    evict_bucket(b, now, ip);

    Entry* e = g_buckets[b];
    while (e)
    {
        if (strcmp(e->ip, ip) == 0)
        {
            if (now - e->window_start >= g_window)
            {
                e->count = 0;
                e->window_start = now;
            }
            return e;
        }
        e = e->next;
    }

    if (g_entries >= MAX_ENTRIES)
    {
        for (unsigned int i = 0; i < BUCKETS; i++) evict_bucket(i, now, NULL);
        if (g_entries >= MAX_ENTRIES) return NULL;
    }

    e = calloc(1, sizeof(Entry));
    if (!e) return NULL;
    snprintf(e->ip, sizeof(e->ip), "%s", ip);
    e->window_start = now;
    e->next = g_buckets[b];
    g_buckets[b] = e;
    g_entries++;
    return e;
}

int ratelimit_middleware(Request* req, Response* res, MiddlewareNode* self, Router* router)
{
    const char* method = str_cstr(&req->method);
    const char* path   = str_cstr(&req->path);

    int guarded = (strcmp(method, "POST") == 0   ||
                   strcmp(method, "PUT") == 0    ||
                   strcmp(method, "PATCH") == 0  ||
                   strcmp(method, "DELETE") == 0)
                  && strncmp(path, "/api/", 5) == 0;
    if (!guarded) return middleware_next(self, req, res, router);

    const char* ip = str_cstr(&req->client_ip);
    if (!ip || !*ip) ip = "unknown";

    long now = (long)time(NULL);
    int over_limit = 0;
    int retry_after = g_window;

    pthread_mutex_lock(&g_lock);
    Entry* e = lookup_or_create(ip, now);
    if (!e)
    {
        over_limit = 1; // table full: fail closed
    }
    else if (e->count >= g_max)
    {
        over_limit = 1;
        retry_after = (int)(g_window - (now - e->window_start));
        if (retry_after < 1) retry_after = 1;
    }
    else
    {
        e->count++;
    }
    pthread_mutex_unlock(&g_lock);

    if (over_limit)
    {
        char ra[32];
        snprintf(ra, sizeof(ra), "%d", retry_after);
        response_status(res, 429, "Too Many Requests");
        response_header(res, "Retry-After", ra);
        response_json(res,
            "{\"ok\":false,\"error\":\"rate limit exceeded, slow down\"}");
        return 1;
    }
    return middleware_next(self, req, res, router);
}

void ratelimit_cleanup(void)
{
    for (int i = 0; i < BUCKETS; i++)
    {
        Entry* e = g_buckets[i];
        while (e)
        {
            Entry* nx = e->next;
            free(e);
            e = nx;
        }
        g_buckets[i] = NULL;
    }
    g_entries = 0;
}
