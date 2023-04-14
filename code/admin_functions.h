#ifndef admin_functions
#define admin_functions  /* Include guard */

#include <netinet/in.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAXLINE 1024

void erro(char *s);
void quitServer(int sockfd);
void quitConsole(int sockfd);
bool deleteUser(char* username);
void handle_admin_console(int sockfd);
void admin_authentication(int sockfd);
void client_authentication(int conn_tcp);
bool addUser(char* username, char* password, char* userType);
void listUsers(int sockfd, int slen, struct sockaddr_in cliaddr);


#endif 