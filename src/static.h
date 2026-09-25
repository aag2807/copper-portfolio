#ifndef STATIC_H
#define STATIC_H

#include "middleware.h"

// Serves one file from disk with Cache-Control, ETag and If-None-Match -> 304.
// Returns 0 (response untouched) when the path is missing or not a regular file.
int static_send_file(Request* req, Response* res, const char* fs_path, const char* content_type);
int static_middleware(Request* req, Response* res, MiddlewareNode* self, Router* router);

#endif
