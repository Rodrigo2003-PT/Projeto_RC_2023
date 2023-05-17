#ifndef client
#define client  /* Include guard */

//User libraries
#include "admin_functions.h"

char *server_address;
int porto_noticias;

typedef struct {
    int multicast_port;
    char* multicast_address;
    int sockfd;
} HandleMulticastArgs;

pthread_t topic_threads[MAX_TOPICS][2];

pthread_t server_thread, multicast_thread;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int multicast_port;

void cleanup(int sig);
void *server_handler(void *arg);
void *multicast_sender_handler(void *arg);
void *multicast_receiver_handler(void *arg);

#endif 