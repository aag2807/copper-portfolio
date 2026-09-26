#ifndef GZIP_H
#define GZIP_H

#include "str.h"

#include <stddef.h>

// Bodies at or below this many bytes are not worth compressing.
#define GZIP_MIN_BYTES 1024

// Does this Accept-Encoding value allow gzip? Honours q=0 and "*".
int gzip_accepted(const char* accept_encoding);
// Is this Content-Type (parameters allowed) a text type worth compressing?
int gzip_compressible(const char* content_type);
// gzip-wraps `len` bytes into `out` (an initialised String, appended to).
// Returns 0 on success, -1 on a zlib error (out is left empty).
int gzip_compress(const void* data, size_t len, String* out);

#endif // !GZIP_H
