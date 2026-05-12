#include "../src/controller.h"
#include "../src/view.h"

#include <stdio.h>

ACTION(index)
{
    ViewData data[] = {{"title", "text", "Welcome"}, {"title", "text", "Welcome"}, {NULL, NULL, NULL}};
    response_status(res, 200, "About");
    render_view(res, "home/about.html", data)
}
