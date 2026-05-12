
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

void render_view(Response* res, const char* template_path, ViewData* data);
char* read_file(const char* path);

#endif // !VIEW_H
