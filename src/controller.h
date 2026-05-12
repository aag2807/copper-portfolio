#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "request.h"
#include "response.h"

typedef void (*ActionFunc)(Request*, Response*, void*);

typedef struct
{
    const char* name;      // "home" "users"
    const char* base_path; // "/" "/users"
} Controller;

// Macro to define action handlers function.
#define ACTION(name) void name(Request* req __attribute__((unused)), \
                               Response* res __attribute__((unused)), \
                               void* ctx __attribute__((unused)))

#endif // !CONTROLLER_H
