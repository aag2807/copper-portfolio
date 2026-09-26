#include "../src/controller.h"
#include "../src/view.h"

ACTION(work_index)
{
    ViewData data[] = {
        {"title", "text", "Work", NULL},
        {"path", "text", str_cstr(&req->path), NULL},
        {"description", "text",
         "Alvaro Guzman's work by domain: payments and banking software, applied AI, systems and languages, web runtimes and gamedev. Career timeline included.",
         NULL},
        {"worknav", "text", "1", NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "work/index.html", data);
}

ACTION(work_fintech_ai)
{
    ViewData data[] = {
        {"title", "text", "Fintech & AI", NULL},
        {"path", "text", str_cstr(&req->path), NULL},
        {"description", "text",
         "On-prem LLM gateway for banking, 60+ payment integrations, RAG and multi-agent orchestration — applied AI in regulated environments, plus a local-first AI lab.",
         NULL},
        {"worknav", "text", "1", NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "work/fintech-ai.html", data);
}

ACTION(work_systems)
{
    ViewData data[] = {
        {"title", "text", "Systems", NULL},
        {"path", "text", str_cstr(&req->path), NULL},
        {"description", "text",
         "c-copper, Cobre, Lunar, Axon — hand-written frameworks, language tooling, and gateways in C, Go, and C#.",
         NULL},
        {"worknav", "text", "1", NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "work/systems.html", data);
}

ACTION(work_web)
{
    ViewData data[] = {
        {"title", "text", "Web", NULL},
        {"path", "text", str_cstr(&req->path), NULL},
        {"description", "text",
         "Browser-side Lua via wasmoon. Per-route module loading, signal-based reactivity, no React, no virtual DOM.",
         NULL},
        {"worknav", "text", "1", NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "work/web.html", data);
}

ACTION(work_gamedev)
{
    ViewData data[] = {
        {"title", "text", "Gamedev", NULL},
        {"path", "text", str_cstr(&req->path), NULL},
        {"description", "text",
         "Passages, a gambit-driven action RPG in Unity; a from-scratch BSP renderer in C; an SDL2 → Rust/Bevy ECS track; LÖVE for prototyping.",
         NULL},
        {"worknav", "text", "1", NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "work/gamedev.html", data);
}

/* 301s for the pre-restructure URLs — old links live on LinkedIn/search. */
ACTION(redirect_systems) { response_redirect_permanent(res, "/work/systems"); }
ACTION(redirect_web)     { response_redirect_permanent(res, "/work/web"); }
ACTION(redirect_gamedev) { response_redirect_permanent(res, "/work/gamedev"); }
ACTION(redirect_ai)      { response_redirect_permanent(res, "/work/fintech-ai"); }
