#include "str.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

String str_new(void)
{
    String s = {0};
    s.cap = 64;
    s.data = malloc(s.cap);
    s.data[0] = '\0';

    return s;
}

String str_from(const char* src)
{
    String s = {0};
    s.len = strlen(src);
    s.cap = s.len + 32;
    s.data = malloc(s.cap);
    memcpy(s.data, src, s.len + 1);

    return s;
}

void str_append(String* s, const char* data)
{
    size_t dlen = strlen(data);
    if (s->len + dlen + 1 > s->cap)
    {
        s->cap = s->len + dlen + 64;
        s->data = realloc(s->data, s->cap);
    }

    memcpy(s->data + s->len, data, dlen + 1);
    s->len += dlen;
}

void str_appendf(String* s, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);

    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0)
    {
        va_end(ap2);
        return;
    }

    size_t need = (size_t)n;
    if (s->len + need + 1 > s->cap)
    {
        s->cap = s->len + need + 64;
        s->data = realloc(s->data, s->cap);
    }

    vsnprintf(s->data + s->len, need + 1, fmt, ap2);
    va_end(ap2);
    s->len += need;
}

void str_free(String* s)
{
    free(s->data);
    s->data = NULL;
    s->len = 0;
    s->cap = 0;
}

char* str_cstr(String* s)
{
    if (s->len + 1 > s->cap)
    {
        s->cap = s->len + 1;
        s->data = realloc(s->data, s->cap);
    }
    s->data[s->len] = '\0';
    return s->data;
}
