#ifndef server
#define server  /* Include guard */

//User libraries
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "admin_functions.h"

#define FIRST_UDP_PORT 5000
#define NUM_ADMIN_THREADS 5
#define MAX_CLIENTS 10
#define PORT_TCP 5500
#define MAXLINE 1024

pthread_t admin_threads[NUM_ADMIN_THREADS];
pthread_t client_threads[MAX_CLIENTS];

void *handle_admin(void *arg);
void *handle_client(void *arg);

#endif 