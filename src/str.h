#ifndef STR_H
#define STR_H

#include <stddef.h>

typedef struct
{
    char* data;
    size_t len;
    size_t cap;
} String;

String str_new(void);
String str_from(const char* s);
void str_append(String* s, const char* data);
void str_append_bytes(String* s, const void* data, size_t len);
#if defined(__GNUC__)
void str_appendf(String* s, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
#else
void str_appendf(String* s, const char* fmt, ...);
#endif
// Appends `data` with & < > " ' replaced by HTML entities.
void str_append_html(String* s, const char* data);
void str_free(String* s);
char* str_cstr(String* s); // null-terminate and return

#endif // !STR_H
