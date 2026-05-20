#include "form.h"

#include <string.h>

static int hex_nybble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

size_t url_decode(const char* src, size_t src_len, char* dst, size_t dst_cap)
{
    size_t di = 0;
    for (size_t i = 0; i < src_len && di + 1 < dst_cap; i++)
    {
        char c = src[i];
        if (c == '+')
        {
            dst[di++] = ' ';
        }
        else if (c == '%' && i + 2 < src_len)
        {
            int hi = hex_nybble(src[i + 1]);
            int lo = hex_nybble(src[i + 2]);
            if (hi >= 0 && lo >= 0)
            {
                dst[di++] = (char)((hi << 4) | lo);
                i += 2;
            }
            else
            {
                dst[di++] = c;
            }
        }
        else
        {
            dst[di++] = c;
        }
    }
    dst[di] = '\0';
    return di;
}

int form_field(const char* body, const char* name, char* out, size_t cap)
{
    if (!body || !name) { if (cap) out[0] = '\0'; return 0; }
    out[0] = '\0';
    size_t nlen = strlen(name);
    const char* p = body;
    while (p && *p)
    {
        const char* eq = strchr(p, '=');
        if (!eq) break;
        const char* amp = strchr(eq + 1, '&');
        size_t value_len = amp ? (size_t)(amp - eq - 1) : strlen(eq + 1);

        if ((size_t)(eq - p) == nlen && strncmp(p, name, nlen) == 0)
        {
            url_decode(eq + 1, value_len, out, cap);
            return 1;
        }
        if (!amp) break;
        p = amp + 1;
    }
    return 0;
}
