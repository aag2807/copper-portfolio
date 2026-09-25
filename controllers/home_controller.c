#include "../src/controller.h"
#include "../src/static.h"
#include "../src/view.h"

#include <stdio.h>

ACTION(home_index)
{
    ViewData data[] = {
        {"title", "text", "Senior Fullstack Engineer — Fintech · .NET · Angular · Go · Odin", NULL},
        {"description", "text",
         "Alvaro Guzman, senior fullstack engineer for fintech and banking: .NET, Angular, Svelte, Vue, Laravel, Go, Odin. 60+ payment integrations, on-prem LLM work.",
         NULL},
        {"path", "text", str_cstr(&req->path), NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/index.html", data);
}

ACTION(home_workshop)
{
    ViewData data[] = {
        {"title", "text", "Workshop", NULL},
        {"path", "text", str_cstr(&req->path), NULL},
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
        {"path", "text", str_cstr(&req->path), NULL},
        {"description", "text",
         "Contact Alvaro Guzman in Santo Domingo (AST, UTC−4) about senior IC, architect and applied-AI roles in fintech. Email, LinkedIn or the form here.",
         NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/contact.html", data);
}

ACTION(home_counter)
{
    ViewData data[] = {
        {"title", "text", "Counter", NULL},
        {"path", "text", str_cstr(&req->path), NULL},
        {"description", "text",
         "A reactive counter in Lua, running in the browser via wasmoon (Lua 5.4 compiled to WASM). One signal, two handlers, no virtual DOM.",
         NULL},
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
        {"path", "text", str_cstr(&req->path), NULL},
        {"description", "text",
         "A todo list in Lua, running in the browser via wasmoon (Lua 5.4 compiled to WASM). Two signals and one effect re-render the list.",
         NULL},
        {"luaModule", "text", "pages/todolist", NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/todolist.html", data);
}

/* Crawler files live in public/; 404 until they exist. */
ACTION(home_robots)
{
    if (!static_send_file(req, res, "public/robots.txt", "text/plain; charset=utf-8"))
        render_not_found(res);
}

ACTION(home_sitemap)
{
    if (!static_send_file(req, res, "public/sitemap.xml", "application/xml; charset=utf-8"))
        render_not_found(res);
}
