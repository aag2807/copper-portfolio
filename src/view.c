#include "view.h"

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

static String render_template(const char* template, ViewData* data, const char* body)
{
    String result = str_new();
    const char* p = template;

    while (*p)
    {
        if (*p == '{' && *(p + 1) == '{')
        {
            p += 2;
            char key[128] = {0};
            int i = 0;
            while (*p && *p != '}' && i < (int)sizeof(key) - 1)
            {
                key[i++] = *p++;
            }
            while (i > 0 && key[i - 1] == ' ')
            {
                key[--i] = '\0';
            }
            if (*p == '}' && *(p + 1) == '}')
            {
                p += 2;
            }

            if (body && strcmp(key, "body") == 0)
            {
                str_append(&result, body);
                continue;
            }

            const char* value = NULL;
            for (int j = 0; data && data[j].key; j++)
            {
                if (strcmp(data[j].key, key) == 0)
                {
                    value = data[j].value;
                    break;
                }
            }
            str_append(&result, value ? value : "");
        }
        else if (*p == '@' && strncmp(p, "@layout", 7) == 0)
        {
            while (*p && *p != '\n') p++;
            if (*p == '\n') p++;
        }
        else
        {
            str_appendf(&result, "%c", *p++);
        }
    }

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
