#include "email.h"

#include "str.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void email_init(void)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void email_cleanup(void)
{
    curl_global_cleanup();
}

// JSON string-escape (writes the surrounding quotes too).
static void json_escape(String* out, const char* s)
{
    str_append(out, "\"");
    for (const char* p = s; *p; p++)
    {
        unsigned char c = (unsigned char)*p;
        switch (c)
        {
            case '"':
                str_append(out, "\\\"");
                break;
            case '\\':
                str_append(out, "\\\\");
                break;
            case '\n':
                str_append(out, "\\n");
                break;
            case '\r':
                str_append(out, "\\r");
                break;
            case '\t':
                str_append(out, "\\t");
                break;
            case '\b':
                str_append(out, "\\b");
                break;
            case '\f':
                str_append(out, "\\f");
                break;
            default:
                if (c < 0x20)
                {
                    str_appendf(out, "\\u%04x", c);
                }
                else
                {
                    str_append_bytes(out, p, 1);
                }
        }
    }
    str_append(out, "\"");
}

static size_t curl_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    String* s = (String*)userdata;
    size_t n = size * nmemb;
    str_append_bytes(s, ptr, n);
    return n;
}

EmailResult email_send_via_resend(const char* api_key, const char* from, const char* to, const char* reply_to, const char* subject, const char* html)
{
    EmailResult result = {0};

    if (!api_key || !*api_key)
    {
        result.ok = 0;
        snprintf(result.message, sizeof(result.message), "RESEND_API_KEY is not set");
        return result;
    }

    String body = str_new();
    str_append(&body, "{");
    str_append(&body, "\"from\":");
    json_escape(&body, from);
    str_append(&body, ",\"to\":[");
    json_escape(&body, to);
    str_append(&body, "]");
    if (reply_to && *reply_to)
    {
        str_append(&body, ",\"reply_to\":");
        json_escape(&body, reply_to);
    }
    str_append(&body, ",\"subject\":");
    json_escape(&body, subject);
    str_append(&body, ",\"html\":");
    json_escape(&body, html);
    str_append(&body, "}");

    String resp = str_new();
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        str_free(&body);
        str_free(&resp);
        result.ok = 0;
        snprintf(result.message, sizeof(result.message), "curl_easy_init failed");
        return result;
    }

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    char auth_hdr[256];
    snprintf(auth_hdr, sizeof(auth_hdr), "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, auth_hdr);

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.resend.com/emails");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str_cstr(&body));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.len);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "c-copper/0.02");

    CURLcode rc = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    result.status_code = (int)http_code;

    if (rc != CURLE_OK)
    {
        result.ok = 0;
        snprintf(result.message, sizeof(result.message), "transport error: %s", curl_easy_strerror(rc));
    }
    else if (http_code >= 200 && http_code < 300)
    {
        result.ok = 1;
        snprintf(result.message, sizeof(result.message), "delivered");
    }
    else
    {
        result.ok = 0;
        // Resend's error body is for the operator only: log it, don't return it.
        fprintf(stderr, "[email] resend %ld: %.400s\n", http_code, str_cstr(&resp));
        snprintf(result.message, sizeof(result.message), "resend returned HTTP %ld", http_code);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    str_free(&body);
    str_free(&resp);
    return result;
}
