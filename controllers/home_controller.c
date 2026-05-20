#include "../src/controller.h"
#include "../src/view.h"

#include <stdio.h>

ACTION(home_index)
{
    ViewData data[] = {
        {"title", "text", "Home", NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/index.html", data);
}

ACTION(home_systems)
{
    ViewData data[] = {
        {"title", "text", "Systems", NULL},
        {"description", "text",
         "c-copper, Cobre, Lunar, Axon — hand-written frameworks, language tooling, and gateways in C, Go, and C#.",
         NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/systems.html", data);
}

ACTION(home_gamedev)
{
    ViewData data[] = {
        {"title", "text", "Gamedev", NULL},
        {"description", "text",
         "LitRPG action prototypes in Unity, BSP rendering research, planned SDL2 and Bevy work.",
         NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/gamedev.html", data);
}

ACTION(home_web)
{
    ViewData data[] = {
        {"title", "text", "Web", NULL},
        {"description", "text",
         "Browser-side Lua via wasmoon. Per-route module loading, signal-based reactivity, no React, no virtual DOM.",
         NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/web.html", data);
}

ACTION(home_ai)
{
    ViewData data[] = {
        {"title", "text", "AI", NULL},
        {"description", "text",
         "Local-first AI tooling. OCR pipeline, RAG bot, multi-agent deliberation, terminal coding assistant — running against a local Ollama daemon.",
         NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/ai.html", data);
}

ACTION(home_workshop)
{
    ViewData data[] = {
        {"title", "text", "Workshop", NULL},
        {"description", "text",
         "Flat index of everything I've built across systems, AI, web, gamedev, and tooling.",
         NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/workshop.html", data);
}

ACTION(home_contact)
{
    ViewData data[] = {
        {"title", "text", "Connect", NULL},
        {"description", "text", "Get in touch with Alvaro Guzman.", NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/contact.html", data);
}

ACTION(home_counter)
{
    ViewData data[] = {
        {"title", "text", "Counter", NULL},
        {"luaModule", "text", "pages/counter", NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/counter.html", data);
}

ACTION(home_todolist)
{
    ViewData data[] = {
        {"title", "text", "Todos", NULL},
        {"luaModule", "text", "pages/todolist", NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/todolist.html", data);
}
