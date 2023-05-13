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
#include <stdio.h>

#define MAXLINE 1024
#define MAX_TOPICS 10
#define MAX_CLIENTS 10


typedef struct client {
    char user_name[50];
    char topics[MAX_TOPICS][512];
    int num_subscribed_topics;
    struct sockaddr_in address;
    struct in_addr multicast_addresses[MAX_TOPICS][16];
} Client;

typedef struct client_node {
    Client client;
    struct client_node* next;
} ClientNode;

typedef struct client_list {
    ClientNode* head;
    int size;
} ClientList;

typedef struct topic {
    struct sockaddr_in subscribed_clients[MAX_CLIENTS];
    char multicast_address[16];
    int num_subscribed_clients;
    char name[50];
} Topic;

typedef struct topic_node {
    Topic topic;
    struct topic_node* next;
} TopicNode;

typedef struct topic_list {
    TopicNode* head;
    int size;
} TopicList;


typedef struct news {
    char title[100];
    char content[1000];
    char timestamp[30];
} News;

sem_t file_semaphore;

// admin_console operations
void quitServer(int sockfd);
void quitConsole(int sockfd);
bool deleteUser(char* username);
void handle_admin_console(int sockfd, ClientList *list, TopicList *list_top);
bool addUser(char* username, char* password, char* userType);
void listUsers(int sockfd, int slen, struct sockaddr_in cliaddr);

// authentication_methods
char* authenticate_client(char *username, char *password);
void admin_authentication(int sockfd, ClientList *list, TopicList *list_top);
void client_authentication(int conn_tcp, ClientList *list, TopicList *list_top);

// client_methods
void handle_leitor_commands(int sockfd, ClientList *list, TopicList *list_top, Client *client);
void handle_jornalista_commands(int sockfd, ClientList *list, TopicList *list_top, Client *client);

// Linked List Client functions
ClientList* createClientList();
void destroyClientList(ClientList* list);
ClientNode* createClientNode(Client client);

ClientList* readClientsFromFile();
void writeClientListToFile(ClientList* list);

void addClient(ClientList* list, Client client);
void removeClient(ClientList* client_list, TopicList* topic_list, char* username);
Client getClientByName(struct sockaddr_in address, char *username, ClientList* list);
struct sockaddr_in get_client_address_by_username(ClientList* client_list, char* username);

void printClientList(ClientList* list);

// LInked List Topic functions
TopicList* newTopicList();
void destroyTopicList(TopicList* topicList);

TopicList* readTopicsFromFile();
void writeTopicListToFile(TopicList* topicList);

void addTopic(TopicList* topicList, Topic newTopic);
void removeTopic(TopicList* topicList, char* topicName);
Topic* getTopicByName(TopicList* topicList, const char* name);

void printTopics(TopicList* topicList);
void receive_news_from_multicast(Client *client);
int isSubscribed(char **topics, int num_topics, char *topic);
void subscribe_topic(TopicList *list_top, Client *client, char* name);

// system_methods
void erro(char *s);

#endif 