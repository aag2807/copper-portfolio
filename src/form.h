#ifndef FORM_H
#define FORM_H

#include <stddef.h>

size_t url_decode(const char* src, size_t src_len, char* dst, size_t dst_cap);
int form_field(const char* body, const char* name, char* out, size_t cap);

#endif // !FORM_H
