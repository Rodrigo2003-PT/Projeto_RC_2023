#ifndef server
#define server  /* Include guard */

//User libraries
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "admin_functions.h"

#define PORT_UDP 9090
#define PORT_TCP 5500
#define MAXLINE 1024

pid_t admin_process, client_process;

#endif 