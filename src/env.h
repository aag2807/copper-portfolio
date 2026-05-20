#ifndef ENV_H
#define ENV_H

void env_load(const char* path);
const char* env_get(const char* key);
void env_cleanup(void);

#endif // !ENV_H
