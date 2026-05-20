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
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/systems.html", data);
}

ACTION(home_gamedev)
{
    ViewData data[] = {
        {"title", "text", "Gamedev", NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/gamedev.html", data);
}

ACTION(home_web)
{
    ViewData data[] = {
        {"title", "text", "Web", NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/web.html", data);
}

ACTION(home_ai)
{
    ViewData data[] = {
        {"title", "text", "AI", NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/ai.html", data);
}

ACTION(home_contact)
{
    ViewData data[] = {
        {"title", "text", "Connect", NULL},
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
