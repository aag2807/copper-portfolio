#ifndef CSRF_H
#define CSRF_H

#include "middleware.h"
#include "router.h"

// Loads the CSRF secret. If CSRF_SECRET is set in the env, uses it as-is.
// Otherwise reads 32 bytes from /dev/urandom and hex-encodes — meaning tokens
// reset on every server start, which is fine for a portfolio.
void csrf_init(void);

// Builds a fresh signed token: "<expires_unix>.<hex-hmac-sha256>".
// Caller passes a buffer at least 96 bytes long.
void csrf_token_make(char* out, size_t cap);

// Pipeline-style middleware. Validates a "csrf_token" form field on POSTs to
// paths starting with the configured prefix (defaults to "/api/"). Other
// methods and paths pass through untouched.
int csrf_middleware(Request* req, Response* res, MiddlewareNode* self, Router* router);

#endif // !CSRF_H
