
#include "../src/controller.h"
#include "../src/model.h"
#include "../src/view.h"

#include <stdio.h>
#include <stdlib.h>

extern User* user_find_by_id(int id);

ACTION(users_list)
{
    static ViewData u1[] = {
        {"id", "text", "1", NULL},
        {"name", "text", "Alvaro", NULL},
        {NULL, NULL, NULL, NULL},
    };
    static ViewData u2[] = {
        {"id", "text", "2", NULL},
        {"name", "text", "Eli", NULL},
        {NULL, NULL, NULL, NULL},
    };
    static ViewData u3[] = {
        {"id", "text", "3", NULL},
        {"name", "text", "Bob", NULL},
        {NULL, NULL, NULL, NULL},
    };
    static ViewData* users[] = {u1, u2, u3, NULL};

    ViewData data[] = {
        {"title", "text", "Users", NULL},
        {"user_count", "text", "3", NULL},
        {"users", "list", NULL, users},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "users/list.html", data);
}

ACTION(users_show)
{
    const char* id_str = request_param(req, "id");
    int id = id_str ? atoi(id_str) : 0;
    char id_buf[16];
    snprintf(id_buf, sizeof(id_buf), "%d", id);
    ViewData data[] = {
        {"title", "text", "User Profile", NULL},
        {"user_id", "text", id_buf, NULL},
        {NULL, NULL, NULL, NULL},
    };
    response_status(res, 200, "OK");
    render_view(res, "users/show.html", data);
}

ACTION(api_users)
{
    response_json(res,
                  "[{\"id\":1,\"name\":\"Alvaro\"},"
                  "{\"id\":2,\"name\":\"Eli\"}]");
}
