#include "env.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    char* key;
    char* value;
} EnvPair;

static EnvPair* g_pairs = NULL;
static int g_count = 0;
static int g_cap = 0;

static char* strip(char* s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    char* end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return s;
}

static void set_pair(const char* key, const char* value)
{
    for (int i = 0; i < g_count; i++)
    {
        if (strcmp(g_pairs[i].key, key) == 0)
        {
            free(g_pairs[i].value);
            g_pairs[i].value = strdup(value);
            return;
        }
    }
    if (g_count >= g_cap)
    {
        g_cap = g_cap ? g_cap * 2 : 16;
        g_pairs = realloc(g_pairs, g_cap * sizeof(EnvPair));
    }
    g_pairs[g_count].key = strdup(key);
    g_pairs[g_count].value = strdup(value);
    g_count++;
}

void env_load(const char* path)
{
    FILE* f = fopen(path, "r");
    if (!f) return;

    char line[2048];
    while (fgets(line, sizeof(line), f))
    {
        char* p = strip(line);
        if (*p == '\0' || *p == '#') continue;

        char* eq = strchr(p, '=');
        if (!eq) continue;

        *eq = '\0';
        char* key = strip(p);
        char* val = strip(eq + 1);

        size_t vlen = strlen(val);
        if (vlen >= 2)
        {
            if ((val[0] == '"' && val[vlen - 1] == '"') ||
                (val[0] == '\'' && val[vlen - 1] == '\''))
            {
                val[vlen - 1] = '\0';
                val++;
            }
        }

        set_pair(key, val);
    }
    fclose(f);
}

const char* env_get(const char* key)
{
    // Look up the .env-loaded value first. If it exists and is non-empty,
    // .env wins (explicit local override). If it's empty/missing, fall through
    // to the process environment — this is the Cloud Run / docker -e path,
    // where .env may carry placeholders only.
    for (int i = 0; i < g_count; i++)
    {
        if (strcmp(g_pairs[i].key, key) == 0)
        {
            if (g_pairs[i].value && *g_pairs[i].value)
                return g_pairs[i].value;
            break;
        }
    }
    return getenv(key);
}

void env_cleanup(void)
{
    for (int i = 0; i < g_count; i++)
    {
        free(g_pairs[i].key);
        free(g_pairs[i].value);
    }
    free(g_pairs);
    g_pairs = NULL;
    g_count = 0;
    g_cap = 0;
}
