#include "../src/controller.h"
#include "../src/view.h"
#include "writing_manifest.h"

#include <string.h>

ACTION(writing_index)
{
    ViewData data[] = {
        {"title", "text", "Writing", NULL},
        {"path", "text", str_cstr(&req->path), NULL},
        {"description", "text",
         "Build logs and long-form writeups — canonical on alvaro-guzman.com, compiled from markdown at build time.",
         NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "writing/index.html", data);
}

ACTION(writing_show)
{
    const char* slug = request_param(req, "slug");
    if (slug)
    {
        for (size_t i = 0; i < sizeof(kWritingPosts) / sizeof(kWritingPosts[0]); i++)
        {
            if (strcmp(slug, kWritingPosts[i].slug) == 0)
            {
                ViewData data[] = {
                    {"title", "text", kWritingPosts[i].title, NULL},
                    {"path", "text", str_cstr(&req->path), NULL},
                    {"description", "text", kWritingPosts[i].desc, NULL},
                    {NULL, NULL, NULL, NULL},
                };
                response_status(res, 200, "OK");
                render_view(res, kWritingPosts[i].view, data);
                return;
            }
        }
    }

    response_status(res, 404, "Not Found");
    response_html(res, "<h1>404 - Page Not Found</h1>");
}
