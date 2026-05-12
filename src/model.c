#include "../src/model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static User users_db[] = {
    {1, "Alvaro", "alvaro@example.com"},
    {2, "Eli", "eli@example.com"},
    {3, "Admin", "admin@example.com"},
};

static int user_count = 3;

User* user_find_by_id(int id)
{
    for (int i = 0; i < user_count; i++)
    {
        if (users_db[i].id == id)
        {
            return &users_db[i];
        }
    }
    return NULL;
}

User* user_find_all(int* count)
{
    *count = user_count;
    return users_db;
}

int user_create(const char* name, const char* email)
{
    if (user_count >= 100)
        return -1;
    User* u = &users_db[user_count++];
    u->id = user_count;
    strncpy(u->name, name, sizeof(u->name) - 1);
    strncpy(u->email, email, sizeof(u->email) - 1);
    return u->id;
}
