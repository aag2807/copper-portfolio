#include "server.h"

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct
{
    Server* server;
    int client_fd;
    struct sockaddr_in client_addr;
} ClientJob;

static void* handle_client(void* arg)
{
}
