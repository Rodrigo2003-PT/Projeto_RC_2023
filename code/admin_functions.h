#ifndef admin_functions
#define admin_functions  /* Include guard */

#include <netinet/in.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAXLINE 1024


void quitConsole();
void erro(char *s);
void quitServer(int sockfd);
bool deleteUser(char* username);
bool addUser(char* username, char* password, char* userType);
void listUsers(int sockfd, int slen, struct sockaddr_in cliaddr);
void handle_admin_console(char *buffer, int sockfd, int slen, struct sockaddr_in cliaddr);
bool admin_authentication(char *buffer, int sockfd, int slen, struct sockaddr_in cliaddr);


#endif 