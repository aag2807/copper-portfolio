
#include "../src/controller.h"
#include "../src/model.h"
#include "../src/view.h"

#include <stdio.h>

extern User* user_find_by_id(int id);

ACTION(users_list)
{
    ViewData data[] = {{"title", "text", "Users"}, {"user_count", "text", "3"}, {NULL, NULL, NULL}};
    response_status(res, 200, "OK");
    render_view(res, "users/list.html", data);
}

ACTION(users_show)
{
    const char* id_str = request_param(req, "id");
    int id = id_str ? atoi(id_str) : 0;
    char id_buf[16];
    snprintf(id_buf, sizeof(id_buf), "%d", id);
    ViewData data[] = {{"title", "text", "User Profile"}, {"user_id", "text", id_buf}, {NULL, NULL, NULL}};
    response_status(res, 200, "OK");
    render_view(res, "users/show.html", data);
}

ACTION(api_users)
{
    response_json(res,
                  "[{\"id\":1,\"name\":\"Alvaro\"},"
                  "{\"id\":2,\"name\":\"Eli\"}]");
}
