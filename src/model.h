#ifndef MODEL_H
#define MODEL_H

typedef struct
{
    int id;
    char name[128];
    char email[256];
} User;

// Model function

User* user_find_by_id(int id);
User* user_find_all(int* count);
int user_create(const char* name, const char* email);

#endif // !MODEL_H
