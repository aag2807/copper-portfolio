#include "gzip.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <zlib.h>

// Parses a q= parameter value; anything malformed counts as q=1.
static double parse_q(const char* p, const char* end)
{
    char buf[16];
    size_t n = (size_t)(end - p);
    if (n == 0 || n >= sizeof(buf)) return 1.0;
    memcpy(buf, p, n);
    buf[n] = '\0';
    char* stop = NULL;
    double q = strtod(buf, &stop);
    if (stop == buf) return 1.0;
    return q;
}

int gzip_accepted(const char* ae)
{
    if (!ae) return 0;

    // q of an explicit "gzip" entry wins over "*"; -1 means "not listed".
    double gzip_q = -1.0, star_q = -1.0;
    const char* p = ae;
    while (*p)
    {
        while (*p == ' ' || *p == '\t' || *p == ',') p++;
        if (!*p) break;
        const char* item_end = p;
        while (*item_end && *item_end != ',') item_end++;

        // Token: up to ';' or whitespace.
        const char* tok_end = p;
        while (tok_end < item_end && *tok_end != ';' && *tok_end != ' ' && *tok_end != '\t') tok_end++;
        size_t tok_len = (size_t)(tok_end - p);

        double q = 1.0;
        for (const char* s = tok_end; s < item_end; s++)
        {
            if (*s != ';') continue;
            s++;
            while (s < item_end && (*s == ' ' || *s == '\t')) s++;
            if (s + 1 < item_end && (s[0] == 'q' || s[0] == 'Q') && s[1] == '=')
            {
                const char* v = s + 2;
                const char* ve = v;
                while (ve < item_end && *ve != ';' && *ve != ' ' && *ve != '\t') ve++;
                q = parse_q(v, ve);
            }
        }

        if ((tok_len == 4 && strncasecmp(p, "gzip", 4) == 0) || (tok_len == 6 && strncasecmp(p, "x-gzip", 6) == 0))
            gzip_q = q;
        else if (tok_len == 1 && *p == '*')
            star_q = q;
        p = item_end;
    }

    if (gzip_q >= 0) return gzip_q > 0;
    return star_q > 0;
}

int gzip_compressible(const char* ct)
{
    if (!ct) return 0;
    while (*ct == ' ') ct++;
    size_t n = 0;
    while (ct[n] && ct[n] != ';' && ct[n] != ' ') n++;

    if (n > 5 && strncasecmp(ct, "text/", 5) == 0) return 1;
    static const char* const kTypes[] = {
        "application/javascript", "application/json", "application/xml", "application/wasm", "image/svg+xml",
    };
    for (size_t i = 0; i < sizeof(kTypes) / sizeof(kTypes[0]); i++)
    {
        if (strlen(kTypes[i]) == n && strncasecmp(ct, kTypes[i], n) == 0) return 1;
    }
    return 0;
}

int gzip_compress(const void* data, size_t len, String* out)
{
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    // windowBits 15 + 16 selects the gzip wrapper instead of raw zlib.
    if (deflateInit2(&zs, 6, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) return -1;

    uLong bound = deflateBound(&zs, (uLong)len);
    unsigned char* buf = malloc(bound);
    if (!buf)
    {
        deflateEnd(&zs);
        return -1;
    }

    zs.next_in = (Bytef*)data;
    zs.avail_in = (uInt)len;
    zs.next_out = buf;
    zs.avail_out = (uInt)bound;
    int rc = deflate(&zs, Z_FINISH);
    size_t produced = bound - zs.avail_out;
    deflateEnd(&zs);

    if (rc != Z_STREAM_END)
    {
        free(buf);
        return -1;
    }
    str_append_bytes(out, buf, produced);
    free(buf);
    return 0;
}
