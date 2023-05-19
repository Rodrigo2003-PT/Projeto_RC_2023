#ifndef server
#define server  /* Include guard */

//User libraries
#include "admin_functions.h"

//TO-DO FINISHED

#define MAXLINE 1024

typedef struct {
    int conn_tcp;
    clientList* list;
    topicList* list_top;
    char* file_config;
} HandleClientArgs;

typedef struct {
    int udp_sockfd;
    clientList* list;
    topicList* list_top;
    char* file_config;
} HandleAdminArgs;

pthread_t admin_thread;
pthread_t client_threads[MAX_CLIENTS];

clientList* list_clients;
topicList* list_topics;

void init_shm();
void cleanup(int sig);
void *handle_admin(void *arg);
void *handle_client(void *arg);

#endif 