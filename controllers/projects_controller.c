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
    {"c-copper", "projects/c-copper.html", "c-copper — a web server in C",
     "Case study: the hand-written C99 server and MVC framework behind this site. Architecture, decisions, measured throughput and live render telemetry."},
    {"llm-gateway", "projects/llm-gateway.html", "On-prem LLM gateway for a bank",
     "Freelance case study: an on-premise LLM gateway for a bank in C#/.NET. Multi-provider routing, RAG and multi-agent orchestration with zero external data egress."},
    {"payment-gateways", "projects/payment-gateways.html", "60+ payment gateway integrations",
     "Case study from ATL Software: checkout, transaction processing and fraud handling across 60+ payment providers; contributed to a +15% conversion."},
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
                    {"sky", "text", "dim", NULL}, /* long-form reading: universe.js dims */
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
