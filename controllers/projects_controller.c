#include "../src/controller.h"
#include "../src/view.h"

#include <string.h>

/* Static allowlist: the slug from the URL is matched here and NEVER passed
 * into a template; {{key}} escapes, but the allowlist keeps unknown slugs out entirely. */
static const struct
{
    const char* slug;
    const char* view;
    const char* title;
    const char* desc;
} kProjects[] = {
    {"c-copper", "projects/c-copper.html", "c-copper — a web framework in C",
     "Case study: the hand-written C99 MVC framework serving this site. Architecture, decisions, and measured throughput."},
    {"llm-gateway", "projects/llm-gateway.html", "On-prem LLM gateway for banking",
     "Case study: multi-provider LLM routing, RAG pipelines, and multi-agent orchestration with zero external data egress."},
    {"payment-gateways", "projects/payment-gateways.html", "60+ payment gateway integrations",
     "Case study: checkout, transaction processing, and fraud handling across 60+ payment providers in regulated fintech."},
};

ACTION(projects_show)
{
    const char* slug = request_param(req, "slug");
    if (slug)
    {
        for (size_t i = 0; i < sizeof(kProjects) / sizeof(kProjects[0]); i++)
        {
            if (strcmp(slug, kProjects[i].slug) == 0)
            {
                ViewData data[] = {
                    {"title", "text", kProjects[i].title, NULL},
                    {"path", "text", str_cstr(&req->path), NULL},
                    {"description", "text", kProjects[i].desc, NULL},
                    {NULL, NULL, NULL, NULL},
                };
                response_status(res, 200, "OK");
                render_view(res, kProjects[i].view, data);
                return;
            }
        }
    }

    render_not_found(res);
}
