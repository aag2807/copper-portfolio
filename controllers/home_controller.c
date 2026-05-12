#include "../src/controller.h"
#include "../src/view.h"

#include <stdio.h>

ACTION(home_index)
{
    ViewData data[] = {{"title", "text", "Welcome"}, {"message", "text", "In C"}};
    response_status(res, 200, "OK");
    render_view(res, "home/index.html", data);
}

ACTION(home_about)
{
    ViewData data[] = {{"title", "text", "About"}, {NULL, NULL, NULL}};
    response_status(res, 200, "OK");
    render_view(res, "home/about.html", data);
}
