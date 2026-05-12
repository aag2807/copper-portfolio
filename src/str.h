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
void str_appendf(String* s, const char* fmt, ...);
void str_free(String* s);
char* str_cstr(String* s); // null-terminate and return

#endif // !STR_H
