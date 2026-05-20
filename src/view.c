#include "view.h"

#include "csrf.h"
#include "str.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

char* read_file(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f)
    {
        return NULL;
    }

    struct stat st;
    if (stat(path, &st) != 0)
    {
        fclose(f);
        return NULL;
    }

    char* buf = malloc(st.st_size + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, st.st_size, f);
    buf[n] = '\0';

    fclose(f);

    return buf;
}

typedef enum
{
    STOP_EOF,
    STOP_ELSE,
    STOP_ENDIF,
    STOP_ENDEACH
} StopTag;

static const char* lookup_value(const char* key, ViewData* data, ViewData* row)
{
    if (row)
    {
        for (int i = 0; row[i].key; i++)
        {
            if (strcmp(row[i].key, key) == 0)
                return row[i].value;
        }
    }
    for (int i = 0; data && data[i].key; i++)
    {
        if (strcmp(data[i].key, key) == 0)
            return data[i].value;
    }
    return NULL;
}

static ViewData** lookup_rows(const char* key, ViewData* data)
{
    for (int i = 0; data && data[i].key; i++)
    {
        if (strcmp(data[i].key, key) == 0 && data[i].type && strcmp(data[i].type, "list") == 0)
            return data[i].rows;
    }
    return NULL;
}

static int is_truthy_value(const char* v)
{
    if (!v || !*v) return 0;
    if (strcmp(v, "0") == 0) return 0;
    if (strcmp(v, "false") == 0) return 0;
    return 1;
}

static int is_truthy_key(const char* key, ViewData* data, ViewData* row)
{
    const char* v = lookup_value(key, data, row);
    if (v) return is_truthy_value(v);
    ViewData** rows = lookup_rows(key, data);
    return rows && rows[0];
}

// Reads an identifier (no spaces) from *pp, stops at space or '}'.
static void read_ident(const char** pp, char* out, size_t cap)
{
    const char* p = *pp;
    size_t i = 0;
    while (*p && *p != ' ' && *p != '}' && i + 1 < cap)
    {
        out[i++] = *p++;
    }
    out[i] = '\0';
    *pp = p;
}

static void skip_spaces(const char** pp)
{
    while (**pp == ' ') (*pp)++;
}

// Consume closing }} if present.
static void close_tag(const char** pp)
{
    skip_spaces(pp);
    if ((*pp)[0] == '}' && (*pp)[1] == '}') *pp += 2;
}

static StopTag render_span(const char** pp, String* out, ViewData* data, ViewData* row, const char* body);

static StopTag discard_span(const char** pp, ViewData* data, ViewData* row, const char* body)
{
    String dummy = str_new();
    StopTag s = render_span(pp, &dummy, data, row, body);
    str_free(&dummy);
    return s;
}

static StopTag render_span(const char** pp, String* out, ViewData* data, ViewData* row, const char* body)
{
    const char* p = *pp;

    while (*p)
    {
        if (p[0] == '{' && p[1] == '{')
        {
            p += 2;
            skip_spaces(&p);

            // Block tags
            if (strncmp(p, "#if", 3) == 0 && (p[3] == ' ' || p[3] == '\t'))
            {
                p += 3;
                skip_spaces(&p);
                char key[128];
                read_ident(&p, key, sizeof(key));
                close_tag(&p);

                int truthy = is_truthy_key(key, data, row);
                StopTag s;
                if (truthy)
                {
                    s = render_span(&p, out, data, row, body);
                    if (s == STOP_ELSE)
                        s = discard_span(&p, data, row, body);
                }
                else
                {
                    s = discard_span(&p, data, row, body);
                    if (s == STOP_ELSE)
                        s = render_span(&p, out, data, row, body);
                }
                (void)s; // expect STOP_ENDIF
                continue;
            }

            if (strncmp(p, "#each", 5) == 0 && (p[5] == ' ' || p[5] == '\t'))
            {
                p += 5;
                skip_spaces(&p);
                char key[128];
                read_ident(&p, key, sizeof(key));
                close_tag(&p);

                ViewData** rows = lookup_rows(key, data);
                const char* body_start = p;
                if (rows && rows[0])
                {
                    for (int r = 0; rows[r]; r++)
                    {
                        const char* iter = body_start;
                        render_span(&iter, out, data, rows[r], body);
                        p = iter;
                    }
                }
                else
                {
                    discard_span(&p, data, row, body);
                }
                continue;
            }

            if (strncmp(p, "/if", 3) == 0)
            {
                p += 3;
                close_tag(&p);
                *pp = p;
                return STOP_ENDIF;
            }

            if (strncmp(p, "/each", 5) == 0)
            {
                p += 5;
                close_tag(&p);
                *pp = p;
                return STOP_ENDEACH;
            }

            if (strncmp(p, "else", 4) == 0 && (p[4] == ' ' || p[4] == '}'))
            {
                p += 4;
                close_tag(&p);
                *pp = p;
                return STOP_ELSE;
            }

            // Plain {{key}} substitution
            char key[128];
            size_t i = 0;
            while (*p && !(p[0] == '}' && p[1] == '}') && i + 1 < sizeof(key))
            {
                key[i++] = *p++;
            }
            key[i] = '\0';
            while (i > 0 && key[i - 1] == ' ') key[--i] = '\0';
            if (p[0] == '}' && p[1] == '}') p += 2;

            if (body && strcmp(key, "body") == 0)
            {
                str_append(out, body);
                continue;
            }

            const char* value = lookup_value(key, data, row);
            if (value) str_append(out, value);
            continue;
        }

        if (p[0] == '@' && strncmp(p, "@layout", 7) == 0)
        {
            while (*p && *p != '\n') p++;
            if (*p == '\n') p++;
            continue;
        }

        // @csrf — emit a hidden input carrying a freshly-signed CSRF token.
        // Boundary check: must not be a prefix of a longer identifier (e.g. @csrfish).
        if (p[0] == '@' && strncmp(p, "@csrf", 5) == 0)
        {
            char c = p[5];
            int is_word_char = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                            || (c >= '0' && c <= '9') || c == '_';
            if (!is_word_char)
            {
                char token[128];
                csrf_token_make(token, sizeof(token));
                str_appendf(out, "<input type=\"hidden\" name=\"csrf_token\" value=\"%s\" />", token);
                p += 5;
                continue;
            }
        }

        str_append_bytes(out, p, 1);
        p++;
    }

    *pp = p;
    return STOP_EOF;
}

static String render_template(const char* template, ViewData* data, const char* body)
{
    String result = str_new();
    const char* p = template;
    render_span(&p, &result, data, NULL, body);
    return result;
}

void render_view(Response* res, const char* template_path, ViewData* data)
{
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "views/%s", template_path);
    char* template = read_file(full_path);

    if (!template)
    {
        response_html(res, "<h1>500 - View not found</h1>");
        return;
    }

    String body = render_template(template, data, NULL);

    char* layout = read_file("views/layout.html");
    if (layout)
    {
        String final = render_template(layout, data, str_cstr(&body));
        response_html(res, str_cstr(&final));
        str_free(&final);
        free(layout);
    }
    else
    {
        response_html(res, str_cstr(&body));
    }
    str_free(&body);
    free(template);
}
