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
         "Long-form writing by Alvaro Guzman on payments, banking software, systems work and engineering judgement. Compiled from markdown at build time.",
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
                    {"ogType", "text", "article", NULL},
                    {"published", "text", kWritingPosts[i].date, NULL},
                    {"sky", "text", "dim", NULL}, /* long-form reading: universe.js dims */
                    {NULL, NULL, NULL, NULL},
                };
                response_status(res, 200, "OK");
                render_view(res, kWritingPosts[i].view, data);
                return;
            }
        }
    }

    render_not_found(res);
}
