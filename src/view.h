
#ifndef VIEW_H
#define VIEW_H

#include "response.h"

typedef struct ViewData
{
    const char* key;
    const char* type;          // "text" or "list"
    const char* value;         // used when type == "text"
    struct ViewData** rows;    // used when type == "list": NULL-terminated array of rows
} ViewData;

#include "str.h"

// Renders views/<template_path> inside views/layout.html.
// {{key}} is HTML-escaped; {{{key}}} is inserted raw; {{body}} in the layout is raw.
void render_view(Response* res, const char* template_path, ViewData* data);
// Sets status 404 and renders views/errors/404.html through the layout.
void render_not_found(Response* res);
// Renders a template string (no layout). Exposed for tests.
String view_render_string(const char* template, ViewData* data, const char* body);
char* read_file(const char* path);

#endif // !VIEW_H
