#ifndef admin_functions
#define admin_functions  /* Include guard */

#include <netinet/in.h>
#include <sys/socket.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#define MAXLINE 1024
#define MAX_TOPICS 10
#define MAX_CLIENTS 10


typedef struct client_struct {
    char user_name[50];
    int num_subscribed_topics;
    struct sockaddr_in address;
    char topics[MAX_TOPICS][512];
    struct in_addr multicast_addresses[MAX_TOPICS][16];
} client_struct;

typedef struct clientNode {
    client_struct client;
    struct clientNode* next;
} clientNode;

typedef struct clientList {
    clientNode* head;
    int size;
} clientList;

typedef struct topic_struct {
    char name[50];
    char multicast_address[16];
    int num_subscribed_clients;
    struct sockaddr_in subscribed_clients[MAX_CLIENTS];
} topic_struct;

typedef struct topicNode {
    topic_struct topic;
    struct topicNode* next;
} topicNode;

typedef struct topicList {
    topicNode* head;
    int size;
} topicList;


typedef struct news_struct {
    char title[100];
    char content[1000];
    char timestamp[30];
} news_struct;

// admin_console operations
void quitServer(int sockfd);
void quitConsole(int sockfd);
bool deleteUser(char* username, char* file);
void handle_admin_console(int sockfd, clientList *list, topicList *list_top, char* file);
bool addUser(char* username, char* password, char* userType, char* file);
void listUsers(int sockfd, int slen, struct sockaddr_in cliaddr, char* file);

// authentication_methods
char* authenticate_client(char *username, char *password, char* file, char* result);
void admin_authentication(int sockfd, clientList *list, topicList *list_top, char* file);
void client_authentication(int conn_tcp, clientList *list, topicList *list_top, char* file);

// client_methods
void handle_leitor_commands(int sockfd, clientList *list, topicList *list_top, client_struct *client);
void handle_jornalista_commands(int sockfd, clientList *list, topicList *list_top, client_struct *client);

// Linked List Client functions
clientList* createClientList();
void destroyClientList(clientList* list);
clientNode* createClientNode(client_struct client);

void addClient(clientList* list, client_struct client);
void removeClient(clientList* client_list, topicList* topic_list, char* username);
client_struct createClient(struct sockaddr_in address, char *username, clientList* list);
struct sockaddr_in getClientAddress(clientList* client_list, char* username);

void printClientList(clientList* list);

// LInked List Topic functions
topicList* newTopicList();
void destroyTopicList(topicList* topic_List);

void addTopic(topicList* topic_List, topic_struct newTopic);
void removeTopic(topicList* topicList, char* topicName);
topic_struct* getTopic(topicList* topic_List, const char* name);
bool existsMulticastTopic(topicList* topic_List, const char* multicast);

void printTopics(topicList* topicList);
void subscribeTopic(topicList *list_top, client_struct *client, char* name, int sockfd);

// system_methods
void erro(char *s);

#endif 