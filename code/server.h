#ifndef server
#define server  /* Include guard */

//User libraries
#include "admin_functions.h"

#define FIRST_UDP_PORT 5000
#define NUM_ADMIN_THREADS 5
#define MAX_CLIENTS 10
#define PORT_TCP 5500
#define MAXLINE 1024

typedef struct {
    int conn_tcp;
    ClientList* list;
    TopicList* list_top;
} HandleClientArgs;

typedef struct {
    int udp_sockfd;
    ClientList* list;
    TopicList* list_top;
} HandleAdminArgs;

pthread_t admin_threads[NUM_ADMIN_THREADS];
pthread_t client_threads[MAX_CLIENTS];

void *handle_admin(void *arg);
void *handle_client(void *arg);

#endif 