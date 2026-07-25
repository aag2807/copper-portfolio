#include "../src/controller.h"
#include "../src/view.h"

#include <stdio.h>

ACTION(home_index)
{
    ViewData data[] = {
        {"title", "text", "Senior Fullstack Engineer — Fintech · .NET · AI", NULL},
        {"description", "text",
         "Alvaro Guzman — senior fullstack engineer for fintech and core banking. .NET, Angular, applied AI/LLM. 60+ payment integrations, on-prem LLM gateway work, and a portfolio served by a hand-written C framework.",
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
        {"path", "text", str_cstr(&req->path), NULL},
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
        {"luaModule", "text", "pages/todolist", NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/todolist.html", data);
}
