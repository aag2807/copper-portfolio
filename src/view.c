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

    fread(buf, 1, st.st_size, f);

    buf[st.st_size] = '\0';

    fclose(f);

    return buf;
}

void render_view(Response* res, const char* template_path, ViewData* data)
{
    // Build full path: views/<template_path>
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "views/%s", template_path);
    char* template = read_file(full_path);

    if (!template)
    {
        response_html(res, "<h1>500 - View not found</h1>");
        return;
    }

    String result = str_new();
    char* p = template;

    while (*p)
    {
        if (*p == '{' && *(p + 1) == '{')
        {
            p += 2; // skip {{
            // Read key
            char key[128] = {0};
            int i = 0;
            while (*p && *p != '}' && i < (int)sizeof(key) - 1)
            {
                key[i++] = *p++;
            }

            // Strip trailing whitespace
            while (i > 0 && key[i - 1] == ' ')
            {
                key[--i] = '\0';
            }

            // Skip }}
            if (*p == '}' && *(p + 1) == '}')
            {
                p += 2;
            }

            // Find value in data
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
            // Layout directive: @layout(main)
            // Skip for now — handled separately
            while (*p && *p != '\n')
            {
                p++;
            }

            if (*p == '\n')
            {
                p++;
            }
        }
        else
        {
            str_appendf(&result, "%c", *p++);
        }
    }

    // Try to wrap in layout
    char* layout = read_file("views/layout.html");
    if (layout)
    {
        // Find {{body}} in layout and replace with content
        String final = str_new();
        char* lp = layout;
        while (*lp)
        {
            if (strncmp(lp, "{{body}}", 8) == 0)
            {
                str_append(&final, str_cstr(&result));
                lp += 8;
            }
            else
            {
                str_appendf(&final, "%c", *lp++);
            }
        }
        response_html(res, str_cstr(&final));
        str_free(&final);
        free(layout);
    }
    else
    {
        response_html(res, str_cstr(&result));
    }
    str_free(&result);
    free(template);
}
