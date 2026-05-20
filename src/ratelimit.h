#ifndef RATELIMIT_H
#define RATELIMIT_H

#include "middleware.h"
#include "router.h"

void ratelimit_init(int max_per_window, int window_seconds);
int ratelimit_middleware(Request* req, Response* res, MiddlewareNode* self, Router* router);
void ratelimit_cleanup(void);

#endif // !RATELIMIT_H
