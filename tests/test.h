#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <string.h>

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                            \
    do                                                                         \
    {                                                                          \
        g_checks++;                                                            \
        if (!(cond))                                                           \
        {                                                                      \
            g_failures++;                                                      \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
        }                                                                      \
    } while (0)

#define CHECK_STR(a, b) CHECK(strcmp((a), (b)) == 0)

#define TEST_DONE()                                                            \
    do                                                                         \
    {                                                                          \
        printf("  %d checks, %d failed\n", g_checks, g_failures);              \
        return g_failures ? 1 : 0;                                             \
    } while (0)

#endif
