#include "csrf.h"
#include "env.h"
#include "form.h"
#include "response.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <openssl/hmac.h>

#define TOKEN_TTL_SECONDS (60 * 60)  // 1 hour
#define HMAC_HEX_LEN      64         // 32 bytes -> 64 hex chars

static char g_secret[129];  // up to 128 hex chars + NUL
static size_t g_secret_len = 0;

static void hex_encode(const unsigned char* in, size_t in_len, char* out)
{
    static const char* hex = "0123456789abcdef";
    for (size_t i = 0; i < in_len; i++)
    {
        out[i * 2]     = hex[(in[i] >> 4) & 0xF];
        out[i * 2 + 1] = hex[in[i] & 0xF];
    }
    out[in_len * 2] = '\0';
}

// Constant-time comparison to avoid timing side-channels on HMAC compare.
static int ct_eq(const char* a, const char* b, size_t n)
{
    unsigned char diff = 0;
    for (size_t i = 0; i < n; i++)
        diff |= (unsigned char)a[i] ^ (unsigned char)b[i];
    return diff == 0;
}

static void hmac_hex(const char* msg, char* out_hex)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    HMAC(EVP_sha256(),
         g_secret, (int)g_secret_len,
         (const unsigned char*)msg, strlen(msg),
         digest, &digest_len);
    hex_encode(digest, digest_len, out_hex);
}

void csrf_init(void)
{
    const char* from_env = env_get("CSRF_SECRET");
    if (from_env && *from_env)
    {
        size_t n = strlen(from_env);
        if (n >= sizeof(g_secret)) n = sizeof(g_secret) - 1;
        memcpy(g_secret, from_env, n);
        g_secret[n] = '\0';
        g_secret_len = n;
        return;
    }

    // No secret configured — generate a random one for this run.
    unsigned char raw[32];
    FILE* f = fopen("/dev/urandom", "rb");
    if (f && fread(raw, 1, sizeof(raw), f) == sizeof(raw))
    {
        hex_encode(raw, sizeof(raw), g_secret);
        g_secret_len = strlen(g_secret);
        fclose(f);
        fprintf(stderr,
            "[csrf] CSRF_SECRET not set; generated an ephemeral one. "
            "Tokens will be invalidated on restart. Set CSRF_SECRET in .env to fix.\n");
        return;
    }
    if (f) fclose(f);

    // Last-resort fallback: weak, but better than empty.
    snprintf(g_secret, sizeof(g_secret), "fallback-%ld", (long)time(NULL));
    g_secret_len = strlen(g_secret);
    fprintf(stderr, "[csrf] WARNING: /dev/urandom unavailable. Using weak fallback secret.\n");
}

void csrf_token_make(char* out, size_t cap)
{
    long expires = (long)time(NULL) + TOKEN_TTL_SECONDS;
    char msg[32];
    snprintf(msg, sizeof(msg), "%ld", expires);

    char mac_hex[HMAC_HEX_LEN + 1];
    hmac_hex(msg, mac_hex);

    snprintf(out, cap, "%s.%s", msg, mac_hex);
}

static int validate_token(const char* token)
{
    if (!token || !*token) return 0;

    const char* dot = strchr(token, '.');
    if (!dot) return 0;

    size_t exp_len = (size_t)(dot - token);
    if (exp_len == 0 || exp_len >= 20) return 0;

    char exp_buf[24] = {0};
    memcpy(exp_buf, token, exp_len);

    for (size_t i = 0; i < exp_len; i++)
        if (!isdigit((unsigned char)exp_buf[i])) return 0;

    long expires = strtol(exp_buf, NULL, 10);
    if (expires < (long)time(NULL)) return 0;

    const char* presented_mac = dot + 1;
    if (strlen(presented_mac) != HMAC_HEX_LEN) return 0;

    char expected_mac[HMAC_HEX_LEN + 1];
    hmac_hex(exp_buf, expected_mac);

    return ct_eq(expected_mac, presented_mac, HMAC_HEX_LEN);
}

static void reject(Response* res, int code, const char* status, const char* json)
{
    response_status(res, code, status);
    response_json(res, json);
}

int csrf_middleware(Request* req, Response* res, MiddlewareNode* self, Router* router)
{
    const char* method = str_cstr(&req->method);
    const char* path   = str_cstr(&req->path);

    // Only guard mutating API endpoints; everything else passes through.
    int guarded = (strcmp(method, "POST") == 0   ||
                   strcmp(method, "PUT") == 0    ||
                   strcmp(method, "PATCH") == 0  ||
                   strcmp(method, "DELETE") == 0)
                  && strncmp(path, "/api/", 5) == 0;
    if (!guarded) return middleware_next(self, req, res, router);

    char token[128] = {0};
    int found = form_field(str_cstr(&req->body), "csrf_token", token, sizeof(token));
    if (!found || !*token)
    {
        const char* hdr = request_header(req, "X-CSRF-Token");
        if (hdr && *hdr)
        {
            size_t n = strlen(hdr);
            if (n >= sizeof(token)) n = sizeof(token) - 1;
            memcpy(token, hdr, n);
            token[n] = '\0';
        }
    }

    if (!validate_token(token))
    {
        reject(res, 403, "Forbidden",
            "{\"ok\":false,\"error\":\"csrf token missing, expired, or invalid\"}");
        return 1;
    }

    return middleware_next(self, req, res, router);
}
