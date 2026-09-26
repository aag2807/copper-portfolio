#include "../src/controller.h"
#include "../src/static.h"
#include "../src/view.h"

#include <stdio.h>

ACTION(home_index)
{
    ViewData data[] = {
        {"title", "text", "Senior Full-Stack Engineer, Payments & Banking · .NET · Angular", NULL},
        {"description", "text",
         "Alvaro Guzman, senior full-stack engineer for payments and banking software: .NET, Angular, TypeScript. 7+ years, 60+ payment integrations. Open to Senior/Staff IC roles; UTC−4.",
         NULL},
        {"path", "text", str_cstr(&req->path), NULL},
        {"alpine", "text", "1", NULL}, /* terminal */
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
         "Everything Alvaro Guzman has built, in one index: payments and banking case studies, the c-copper C server, AI tools, web runtimes and games.",
         NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/workshop.html", data);
}

ACTION(home_contact)
{
    ViewData data[] = {
        {"title", "text", "Contact", NULL},
        {"path", "text", str_cstr(&req->path), NULL},
        {"description", "text",
         "Hire Alvaro Guzman, senior full-stack engineer (Santo Domingo, UTC−4): Senior/Staff IC, full-time via employer of record or long-term contract. Replies within 24 hours.",
         NULL},
        {"alpine", "text", "1", NULL}, /* contact form */
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

ACTION(home_playground)
{
    ViewData data[] = {
        {"title", "text", "Lua playground", NULL},
        {"path", "text", str_cstr(&req->path), NULL},
        {"description", "text",
         "Edit and run Lua 5.4 in the browser: wasmoon compiles the reference interpreter to WebAssembly. Live DOM preview, console, instruction-limit guard.",
         NULL},
        {"luaModule", "text", "pages/playground", NULL},
        {"sky", "text", "dim", NULL}, /* dense page: universe.js dims */
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/playground.html", data);
}

ACTION(home_reconcile)
{
    ViewData data[] = {
        {"title", "text", "Reconciliation checker", NULL},
        {"path", "text", str_cstr(&req->path), NULL},
        {"description", "text",
         "Match an internal payments ledger against a provider settlement report, by reference, in Lua running in the browser. Planted discrepancies, live verdicts, integer cents.",
         NULL},
        {"luaModule", "text", "pages/reconcile", NULL},
        {"sky", "text", "dim", NULL}, /* dense page: universe.js dims */
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "home/reconcile.html", data);
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
