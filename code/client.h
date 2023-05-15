#ifndef client
#define client  /* Include guard */

//User libraries
#include "admin_functions.h"

char *server_address;
int porto_noticias;

pthread_t topic_threads[MAX_TOPICS];

pthread_t server_thread, multicast_thread;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *server_handler(void *arg);
void *multicast_handler(void *arg);

#endif 