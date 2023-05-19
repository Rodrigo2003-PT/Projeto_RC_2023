#ifndef client
#define client  /* Include guard */

//User libraries
#include "admin_functions.h"

char *server_address;
int porto_noticias;

typedef struct {
    int multicast_port;
    char multicast_address[16];
} HandleMulticastArgs;

pthread_t topic_threads[MAX_TOPICS];

pthread_t server_thread;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int multicast_port;

void cleanup(int sig);
void *server_handler(void *arg);
void *multicast_receiver_handler(void *arg);
void addEntry(int port, int sockfd, const char* address);
void multicast_send(int sockfd, int multicast_port, char* group_address, char* message);

#endif 