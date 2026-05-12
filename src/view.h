
#ifndef VIEW_H
#define VIEW_H

#include "response.h"

typedef struct
{
    const char* key;
    const char* type;
    const char* value;
} ViewData;

void render_view(Response* res, const char* template_path, ViewData* data);
char* read_file(const char* path);

#endif // !VIEW_H
