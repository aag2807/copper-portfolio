#include "../src/controller.h"
#include "../src/email.h"
#include "../src/env.h"
#include "../src/form.h"
#include "../src/response.h"
#include "../src/str.h"

#include <stdio.h>
#include <string.h>

static void html_escape(String* out, const char* s)
{
    for (const char* p = s; *p; p++)
    {
        switch (*p)
        {
            case '<':  str_append(out, "&lt;");  break;
            case '>':  str_append(out, "&gt;");  break;
            case '&':  str_append(out, "&amp;"); break;
            case '"':  str_append(out, "&quot;"); break;
            case '\n': str_append(out, "<br>");  break;
            default:   str_append_bytes(out, p, 1);
        }
    }
}

static const char* reason_for(int status)
{
    switch (status)
    {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        default:  return "Error";
    }
}

static void respond_json(Response* res, int status, const char* json)
{
    response_status(res, status, reason_for(status));
    response_json(res, json);
}

/* One plain address (local@domain.tld) and nothing else: it becomes the
 * Resend reply_to, so lists, display names and header tricks are refused. */
static int is_single_email(const char* s)
{
    size_t len = strlen(s);
    if (len < 3 || len > 254) return 0;

    const char* at = NULL;
    for (const char* p = s; *p; p++)
    {
        unsigned char c = (unsigned char)*p;
        if (c <= 0x20 || c >= 0x7f) return 0;
        if (strchr(",;<>()[]\\\":", c)) return 0;
        if (c == '@')
        {
            if (at) return 0;
            at = p;
        }
    }
    if (!at || at == s || at[1] == '\0') return 0;

    const char* domain = at + 1;
    const char* dot = strrchr(domain, '.');
    if (!dot || dot == domain || dot[1] == '\0') return 0;
    if (domain[0] == '.' || domain[strlen(domain) - 1] == '.' || strstr(domain, "..")) return 0;
    return 1;
}

ACTION(contact_submit)
{
    const char* body = str_cstr(&req->body);
    if (!body || !*body)
    {
        respond_json(res, 400, "{\"ok\":false,\"error\":\"empty body\"}");
        return;
    }

    char name[256], email[256], message[4096];
    int have_name    = form_field(body, "name",    name,    sizeof(name));
    int have_email   = form_field(body, "email",   email,   sizeof(email));
    int have_message = form_field(body, "message", message, sizeof(message));

    if (!have_name || !have_email || !have_message
        || !name[0] || !email[0] || !message[0])
    {
        respond_json(res, 400,
            "{\"ok\":false,\"error\":\"name, email and message are required\"}");
        return;
    }

    if (!is_single_email(email))
    {
        respond_json(res, 400, "{\"ok\":false,\"error\":\"a single valid email address is required\"}");
        return;
    }

    const char* api_key = env_get("RESEND_API_KEY");
    const char* from    = env_get("RESEND_FROM");
    const char* to      = env_get("RESEND_TO");
    if (!api_key || !*api_key || !from || !to)
    {
        respond_json(res, 500,
            "{\"ok\":false,\"error\":\"server is not configured: missing .env values\"}");
        return;
    }

    char subject[384];
    snprintf(subject, sizeof(subject), "[c-copper] new contact from %s", name);

    String html = str_new();
    str_append(&html, "<div style=\"font-family:monospace;font-size:13px;line-height:1.5\">");
    str_append(&html, "<p><strong>From:</strong> ");
    html_escape(&html, name);
    str_append(&html, " &lt;");
    html_escape(&html, email);
    str_append(&html, "&gt;</p><hr><p>");
    html_escape(&html, message);
    str_append(&html, "</p></div>");

    EmailResult r = email_send_via_resend(api_key, from, to, email, subject, str_cstr(&html));
    str_free(&html);

    if (r.ok)
    {
        respond_json(res, 200, "{\"ok\":true}");
    }
    else
    {
        // Provider details stay in the logs; the client only learns it failed.
        fprintf(stderr, "[contact] email delivery failed (status %d): %s\n", r.status_code, r.message);
        respond_json(res, 502, "{\"ok\":false,\"error\":\"email delivery failed, please try again later\"}");
    }
}
