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

static void respond_json(Response* res, int status, const char* json)
{
    response_status(res, status, status == 200 ? "OK" : "Bad Request");
    response_json(res, json);
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
        String resp = str_new();
        str_append(&resp, "{\"ok\":false,\"status\":");
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", r.status_code);
        str_append(&resp, buf);
        str_append(&resp, ",\"error\":\"");
        for (const char* p = r.message; *p; p++)
        {
            if (*p == '"' || *p == '\\') str_append(&resp, "\\");
            if (*p == '\n' || *p == '\r' || *p == '\t') { str_append(&resp, " "); continue; }
            str_append_bytes(&resp, p, 1);
        }
        str_append(&resp, "\"}");
        respond_json(res, 502, str_cstr(&resp));
        str_free(&resp);
    }
}
