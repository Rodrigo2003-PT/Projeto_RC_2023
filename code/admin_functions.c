#include "admin_functions.h"

// TO-DO FINISHED

pthread_mutex_t topic_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t admin_file_mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_admin_console(int sockfd, clientList *list, topicList *list_top, char* file){

    int recv_len;
    char buffer[MAXLINE];
    struct sockaddr_in cliaddr;
    socklen_t slen = sizeof(cliaddr);

    while (1) {

        // Receive command from admin console
        if((recv_len = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr *) &cliaddr, (socklen_t *)&slen)) == -1) {
            perror("ERRO NO RECVFROM\n");
            quitConsole(sockfd);
            break;
        }

        buffer[recv_len] = '\0';

        // Parse the command
        char* token = strtok(buffer, " ");

        if (strcmp(token, "ADD_USER") == 0) {
            char* username = strtok(NULL, " ");
            char* password = strtok(NULL, " ");
            char* userType = strtok(NULL, " \n");

            pthread_mutex_lock(&admin_file_mutex);

            if (addUser(username, password, userType, file)) {
                sendto(sockfd, "USER_ADDED_SUCCESSFULY\n", strlen("USER_ADDED_SUCCESSFULY\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            } else {
                sendto(sockfd, "FAILED_TO_ADD_USER\n", strlen("FAILED_TO_ADD_USER\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
                }
            
            pthread_mutex_unlock(&admin_file_mutex);
        } 
        else if (strcmp(token, "DEL") == 0) {

            char* username = strtok(NULL, " \n");

            pthread_mutex_lock(&admin_file_mutex);

            if (deleteUser(username, file)) {
                removeClient(list,list_top,username);
                sendto(sockfd, "USER_DELETED_SUCCESSFULY\n", strlen("USER_DELETED_SUCCESSFULY\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            }
            else {
                sendto(sockfd, "FAILED_TO_DELETE_USER\n", strlen("FAILED_TO_DELETE_USER\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            }

            pthread_mutex_unlock(&admin_file_mutex);
        } 
        else if (strcmp(token, "LIST\n") == 0) {
            pthread_mutex_lock(&admin_file_mutex);
            listUsers(sockfd,slen,cliaddr,file);
            pthread_mutex_unlock(&admin_file_mutex);
        }
        else if (strcmp(token, "QUIT\n") == 0) {
            quitConsole(sockfd);
            break;
        } 
        else if (strcmp(token, "QUIT_SERVER\n") == 0) {
            quitServer(sockfd);
        }
        else {
            sendto(sockfd, "INVALID_COMMAND\n", strlen("INVALID_COMMAND\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
        }
    }
}

void handle_leitor_commands(int sockfd, clientList *list, topicList *list_top, client_struct *client) {

    char* token;
    char buffer[MAXLINE];

    while (1) {
        // Receive command from client
        if (recv(sockfd, buffer, MAXLINE, 0) == -1) {
            perror("ERROR_RECEIVING_FROM_CLIENT\n");
            close(sockfd);
            break;
        }

        // Parse command
        token = strtok(buffer, " \n");

        if (strcmp(token, "LIST_TOPICS") == 0) {

            pthread_mutex_lock(&topic_mutex);
            topicNode *cur_topic = list_top->head;
            char topic_str[MAXLINE] = "";

            while (cur_topic != NULL) {
                strcat(topic_str, cur_topic->topic.name);
                strcat(topic_str, "\n");
                cur_topic = cur_topic->next;
            }
            pthread_mutex_unlock(&topic_mutex);

            if (send(sockfd, topic_str, strlen(topic_str), 0) == -1) {
                perror("ERROR_SENDING_MESSAGE_TO_CLIENT\n");
                exit(EXIT_FAILURE);
            }
        } 
        else if (strcmp(token, "SUBSCRIBE_TOPIC") == 0) {
            token = strtok(NULL, " \n");
            subscribeTopic(list_top,client,token,sockfd);
        }
        else {
            // Invalid command
            if (send(sockfd, "INVALID_COMMAND\n", 17, 0) == -1) {
                perror("ERROR_SENDING_MESSAGE_TO_CLIENT\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void handle_jornalista_commands(int sockfd, clientList *list, topicList *list_top, client_struct *client){
    char* token;
    char buffer[MAXLINE];

    while (1) {
        // Receive command from client
        if (recv(sockfd, buffer, MAXLINE, 0) == -1) {
            perror("ERROR_RECEIVING_COMMAND_FROM_CLIENT\n");
            close(sockfd);
            break;
        }

        // Parse command
        token = strtok(buffer, " \n");

        if (strcmp(token, "LIST_TOPICS") == 0) {
            pthread_mutex_lock(&topic_mutex);
            topicNode *cur_topic = list_top->head;
            char topic_str[MAXLINE] = "";

            while (cur_topic != NULL) {
                strcat(topic_str, cur_topic->topic.name);
                strcat(topic_str, "\n");
                cur_topic = cur_topic->next;
            }
            pthread_mutex_unlock(&topic_mutex);

            if (send(sockfd, topic_str, strlen(topic_str), 0) == -1) {
                perror("ERROR_SENDING_MESSAGE_TO_CLIENT\n");
                exit(EXIT_FAILURE);
            }
        } 
        else if (strcmp(token, "SUBSCRIBE_TOPIC") == 0) {
            token = strtok(NULL, " \n");
            subscribeTopic(list_top,client,token,sockfd);
        }
        else if(strcmp(token, "CREATE_TOPIC") == 0){
            char* multicast_id = strtok(NULL, " \n");
            char* topic_title = strtok(NULL, "\n");
            
            topic_struct new_topic;
            strcpy(new_topic.name,topic_title);
            strcpy(new_topic.multicast_address, multicast_id);
            memset(new_topic.subscribed_clients, 0, sizeof(new_topic.subscribed_clients));
            new_topic.num_subscribed_clients = 0;

            pthread_mutex_lock(&topic_mutex);
            addTopic(list_top,new_topic);
            pthread_mutex_unlock(&topic_mutex);

            // Send a confirmation message to the client
            char msg[MAXLINE];
            snprintf(msg, MAXLINE, "TOPIC_CREATED: %s\n", new_topic.name);
            if (send(sockfd, msg, strlen(msg), 0) == -1) {
                perror("ERROR_SENDING_MESSAGE_TO_CLIENT\n");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(token,"SEND_NEWS") == 0){
            char* multicast_id = strtok(NULL," ");

            if(existsMulticastTopic(list_top,multicast_id)){
                char msg[MAXLINE];
                strcpy(msg,"SUCCESSFULL");
                if (send(sockfd, msg, strlen(msg), 0) == -1) {
                    perror("ERROR_SENDING_MESSAGE_TO_CLIENT\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
        else {
            // Invalid command
            if (send(sockfd, "INVALID_COMMAND\n", 17, 0) == -1) {
                perror("ERROR_SENDING_MESSAGE_TO_CLIENT\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void admin_authentication(int sockfd, clientList *list, topicList *list_top, char* file){

    int recv_len;
    char buffer[MAXLINE];
    bool authenticated = false;
    struct sockaddr_in cliaddr;
    socklen_t slen = sizeof(cliaddr);

    while (!authenticated) {

        // Receive credentials from admin console
        if((recv_len = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr *) &cliaddr, (socklen_t *)&slen)) == -1) {
            perror("ERRO NO RECVFROM\n");
            close(sockfd);
            break;
        }
        
        // Para ignorar o restante conteúdo (anterior do buffer)
	    buffer[recv_len]='\0';

        // Parse the credentials
        char* username = strtok(buffer, " ");
        char* password = strtok(NULL, " \n");

        // Verify credentials
        if (strcmp(username, "admin") == 0 && strcmp(password, "password") == 0) {
            authenticated = true;
            sendto(sockfd, "AUTHENTICATION SUCCESSFUL\n", strlen("AUTHENTICATION SUCCESSFUL\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            handle_admin_console(sockfd, list, list_top, file);
        } 
        else {
            sendto(sockfd, "AUTHENTICATION FAILED: <username> <password>\n", strlen("AUTHENTICATION FAILED <username> <password>\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
        }
    }
}

void client_authentication(int sockfd, clientList *list, topicList *list_top, char* file){

    char* token;
    int recv_len;
    char* result = NULL;
    client_struct client;
    char buffer[MAXLINE];
    bool authenticated = false;
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

     while (!authenticated) {

        // Receive credentials from client
        if((recv_len = recv(sockfd, buffer, MAXLINE, 0)) == -1) {
            perror("ERRO NO RECV\n");
            close(sockfd);
            break;
        }
        
        // Para ignorar o restante conteúdo (anterior do buffer)
	    buffer[recv_len]='\0';

        // Parse the credentials
        char* username = strtok(buffer, " ");
        char* password = strtok(NULL, " \n");

        // Verify credentials
        if ((token = authenticate_client(username,password,file,result)) != NULL) {
            authenticated = true;
            free(result);
            send(sockfd, "AUTHENTICATION_SUCCESSFUL\n", strlen("AUTHENTICATION_SUCCESSFUL\n"), 0);

            if(getpeername(sockfd, (struct sockaddr *)&client_address, &client_address_len) == -1){
                printf("ERROR_GETING_CLIENT_ADDRESS\n");
                close(sockfd);
                return;
            }

            client = createClient(client_address, username, list);

            if (strcmp(token, "leitor") == 0) {
                handle_leitor_commands(sockfd, list, list_top, &client);
            } 
            else if (strcmp(token, "jornalista") == 0) {
                handle_jornalista_commands(sockfd,list, list_top, &client);
            }
        } 
        else {
            send(sockfd, "WRONG_CREDENTIALS\n", strlen("WRONG_CREDENTIALS\n"),0);
        }
    }
};

char* authenticate_client(char *username, char *password, char* file, char* result) {

    char line[MAXLINE];
    char *token;
    FILE *fp;

    // Open file for reading
    pthread_mutex_lock(&admin_file_mutex);
    fp = fopen(file, "r");
    if (fp == NULL) {
        perror("ERROR OPENING FILE\n");
        exit(EXIT_FAILURE);
    }

    // Read each line of the file
    while (fgets(line, MAXLINE, fp) != NULL) {

        // Get username
        token = strtok(line, ";");
        if (strcmp(token, username) != 0) continue; // Skip if username does not match

        // Get password
        token = strtok(NULL, ";");
        if (strcmp(token, password) != 0) continue; // Skip if password does not match

        // Get user type
        token = strtok(NULL, "\n");
        result = strdup(token);

        fclose(fp);
        pthread_mutex_unlock(&admin_file_mutex);
        return result; // Authentication successful
    }

    fclose(fp);
    pthread_mutex_unlock(&admin_file_mutex);
    return NULL; // Authentication failed
}

// Function to handle ADD_USER command
bool addUser(char* username, char* password, char* userType, char* file) {

    FILE *fp;
    char line[MAXLINE];
    char* token;
    bool userExists = false;
    
    // Open the file in read mode
    fp = fopen(file, "r");

    // Check if the file was opened successfully
    if (fp == NULL) {
        printf("ERROR_OPENING_FILE\n");
        return false;
    }

    // Loop through the file line by line
    while (fgets(line, MAXLINE, fp) != NULL) {

        // Split the line using strtok() to extract username, password, and userType
        token = strtok(line, ";");
        char* existingUsername = token;

        // Check if the username already exists in the file
        if (strcmp(username, existingUsername) == 0) {
            userExists = true;
            break;
        }
    }

    // Close the file
    fclose(fp);

    // If user already exists, return false
    if (userExists) {
        printf("USER_EXISTS\n");
        return false;
    }

    // Open the file in append mode
    fp = fopen(file, "a");

    // Check if the file was opened successfully
    if (fp == NULL) {
        printf("ERROR_OPENING_FILE\n");
        return false;
    }

    // Write the new user information to the file
    fprintf(fp, "%s;%s;%s\n", username, password, userType);

    // Close the file
    fclose(fp);

    // Return true if the user was added successfully
    return true;
}

// Function to handle DEL command
bool deleteUser(char* username, char* file) {

    char tempFile[] = "temp.txt";
    char buffer[256];

    FILE *fp = fopen(file, "r");
    FILE *temp = fopen(tempFile, "w");

    // Check if file could be opened
    if (fp == NULL || temp == NULL) {
        printf("ERROR_OPENING_FILE\n");
        return false;
    }

    bool found = false;

    // Read through file line by line
    while (fgets(buffer, 256, fp) != NULL) {
        
        char* line = strdup(buffer); // duplicate the line
        char* name = strtok(line, ";"); // tokenize the duplicate line

        if (strcmp(name, username) == 0) {
            found = true;
        } else {
            fputs(buffer, temp);
        }

        free(line); // free the duplicated line
    }

    fclose(fp);
    fclose(temp);

    // Delete original file and rename temp file
    remove(file);
    rename(tempFile, file);

    if (found) {
        printf("USER_DELETED_SUCCESSFULY\n");
        return true;
    } else {
        printf("USER_NOT_FOUND\n");
        return false;
    }
}

// Function to handle LIST command
// Can be done iterating through client_list
void listUsers(int sockfd, int slen, struct sockaddr_in cliaddr, char* file) {

    FILE* fp;
    char* token;
    char line[MAXLINE];

    fp = fopen(file, "r");

    if (fp == NULL) {
        printf("ERROR: COULD_NOT_OPEN_FILE\n");
        return;
    }

    while (fgets(line, MAXLINE, fp) != NULL) {
        token = strtok(line, ";");
        sendto(sockfd, token, strlen(token), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
        token = strtok(NULL, ";");
        sendto(sockfd, token, strlen(token), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
        token = strtok(NULL, ";");
        sendto(sockfd, token, strlen(token), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
    }

    fclose(fp);
}

// Function to handle QUIT command
void quitConsole(int sockfd) {
    printf("ADMIN CONSOLE SESSION ENDED\n");
    close(sockfd);
}

// Function to handle QUIT_SERVER command
void quitServer(int sockfd) {
    printf("SERVER_SHUTTING_DOWN\n");
    close(sockfd);
    exit(EXIT_SUCCESS);
}

void erro(char *s) {
	perror(s);
	exit(1);
}

// Function to create a new client node
clientNode* createClientNode(client_struct client) {
    clientNode* node = (clientNode*) malloc(sizeof(clientNode));
    if (node == NULL) {
        fprintf(stderr, "ERROR: COULD NOT ALLOCATE MEMORY FOR CLIENT_NODE\n");
        exit(EXIT_FAILURE);
    }
    node->client = client;
    node->next = NULL;
    return node;
}


// Function to create a new client list
clientList* createClientList() {
    clientList* list = (clientList*) malloc(sizeof(clientList));
    if (list == NULL) {
        fprintf(stderr, "ERROR: COULD NOT ALLOCATE MEMORY FOR CLIENT LIST\n");
        exit(EXIT_FAILURE);
    }
    list->head = NULL;
    list->size = 0;
    return list;
}


void addClient(clientList* list, client_struct client) {
    clientNode* new_node = createClientNode(client);

    if (list->head == NULL) {
        // list is empty
        list->head = new_node;
    } else {
        // add new node to the end of the list
        clientNode* current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }

    // increment the size of the list
    list->size++;
}


void removeClient(clientList* client_list, topicList* topic_list, char *username) {
    // Remove client from client list
    clientNode* curr_client = client_list->head;
    clientNode* prev_client = NULL;

    pthread_mutex_lock(&client_mutex);
    struct sockaddr_in address = getClientAddress(client_list, username);

    while (curr_client != NULL) {
        if (memcmp(&(curr_client->client.user_name), &username, sizeof(struct sockaddr_in)) == 0) {
            if (prev_client == NULL) {
                client_list->head = curr_client->next;
            } else {
                prev_client->next = curr_client->next;
            }
            free(curr_client);
            client_list->size--;
            break;
        }
        prev_client = curr_client;
        curr_client = curr_client->next;
    }
    pthread_mutex_unlock(&client_mutex);

    pthread_mutex_lock(&topic_mutex);
    // Remove client from topic subscriber lists
    topicNode* curr_topic = topic_list->head;
    while (curr_topic != NULL) {
        int i;
        for (i = 0; i < curr_topic->topic.num_subscribed_clients; i++) {
            if (memcmp(&(curr_topic->topic.subscribed_clients[i]), &address, sizeof(struct sockaddr_in)) == 0) {
                int j;
                for (j = i; j < curr_topic->topic.num_subscribed_clients - 1; j++) {
                    curr_topic->topic.subscribed_clients[j] = curr_topic->topic.subscribed_clients[j + 1];
                }
                curr_topic->topic.num_subscribed_clients--;
                break;
            }
        }
        curr_topic = curr_topic->next;
    }
    pthread_mutex_unlock(&topic_mutex);
}

struct sockaddr_in getClientAddress(clientList* client_list, char* username) {
    clientNode* curr_client = client_list->head;
    while (curr_client != NULL) {
        if (strcmp(curr_client->client.user_name, username) == 0) {
            return curr_client->client.address;
        }
        curr_client = curr_client->next;
    }
    struct sockaddr_in empty_address = {0};
    return empty_address;
}

void destroyClientList(clientList* list) {
    clientNode* current = list->head;
    while (current != NULL) {
        clientNode* temp = current;
        current = current->next;
        free(temp);
    }
    list->head = NULL;
    list->size = 0;
}

client_struct createClient(struct sockaddr_in address, char *username, clientList* list) {

    client_struct client;
    strcpy(client.user_name, username);
    client.address = address;
    client.num_subscribed_topics = 0;
    memset(client.topics, 0, sizeof(client.topics));
    memset(client.multicast_addresses, 0, sizeof(client.multicast_addresses));
    pthread_mutex_lock(&client_mutex);
    addClient(list, client);
    pthread_mutex_unlock(&client_mutex);
    return client;
}

void printClientList(clientList* list) {
    pthread_mutex_lock(&client_mutex);
    clientNode* current = list->head;
    printf("CLIENT_LIST:\n");
    printf("------------\n");
    while (current != NULL) {
        printf("USERNAME: %s\n", current->client.user_name);
        printf("NUMER OF SUBSCRIBED TOPICS: %d\n", current->client.num_subscribed_topics);
        printf("SUBSCRIBED_TOPICS: ");
        for (int i = 0; i < current->client.num_subscribed_topics; i++) {
            printf("%s ", current->client.topics[i]);
        }
        printf("\n");
        printf("ADDRESS: %s\n", inet_ntoa(current->client.address.sin_addr));
        printf("MULTICAST_ADDRESSES: ");
        for (int i = 0; i < current->client.num_subscribed_topics; i++) {
            char multicast_str[INET_ADDRSTRLEN];
            for (int j = 0; j < 16; j++) {
                if (inet_ntop(AF_INET, &(current->client.multicast_addresses[i][j]), multicast_str, INET_ADDRSTRLEN) == NULL) {
                    perror("ERROR -> CONVERTING MULTICAST TO STRING\n");
                    exit(EXIT_FAILURE);
                }
                printf("%s ", multicast_str);
            }
        }
        printf("\n\n");
        current = current->next;
    }
    pthread_mutex_unlock(&client_mutex);
}

topicList* newTopicList() {
    topicList* topic_List = (topicList*) malloc(sizeof(topicList));
    topic_List->head = NULL;
    topic_List->size = 0;
    return topic_List;
}

void destroyTopicList(topicList* topic_List) {
    if (topic_List == NULL) {
        return;
    }

    topicNode* currentNode = topic_List->head;
    topicNode* nextNode;

    while (currentNode != NULL) {
        nextNode = currentNode->next;
        free(currentNode);
        currentNode = nextNode;
    }

    free(topic_List);
}

void addTopic(topicList* topic_List, topic_struct newTopic) {
    topicNode* newNode = (topicNode*) malloc(sizeof(topicNode));
    newNode->topic = newTopic;
    newNode->next = NULL;

    if (topic_List->head == NULL) {
        topic_List->head = newNode;
    } else {
        topicNode* currentNode = topic_List->head;
        while (currentNode->next != NULL) {
            currentNode = currentNode->next;
        }
        currentNode->next = newNode;
    }

    topic_List->size++;
}


void removeTopic(topicList* list, char* topic_name) {
    // Check if the list is empty
    if (list->head == NULL) {
        printf("THE TOPIC IS EMPTY\n");
        return;
    }
    // Check if the head node is the target node
    if (strcmp(list->head->topic.name, topic_name) == 0) {
        topicNode* temp = list->head;
        list->head = list->head->next;
        free(temp);
        list->size--;
        return;
    }
    // Traverse the list to find the target node
    topicNode* current = list->head;
    while (current->next != NULL && strcmp(current->next->topic.name, topic_name) != 0) {
        current = current->next;
    }
    // Check if the target node was found
    if (current->next == NULL) {
        printf("TOPIC_NOT_FOUND\n");
        return;
    }
    // Remove the target node
    topicNode* temp = current->next;
    current->next = current->next->next;
    free(temp);
    list->size--;
}

topic_struct* getTopic(topicList* topic_List, const char* name) {
    topicNode* currentNode = topic_List->head;
    while (currentNode != NULL) {
        if (strcmp(currentNode->topic.name, name) == 0) {
            return &(currentNode->topic);
        }
        currentNode = currentNode->next;
    }
    return NULL;
}

bool existsMulticastTopic(topicList* topic_List, const char* multicast){
    topicNode* currentNode = topic_List->head;
    while (currentNode != NULL) {
        if (strcmp(currentNode->topic.multicast_address, multicast) == 0) {
            return true;
        }
        currentNode = currentNode->next;
    }
    return false;
}

void printTopics(topicList* list) {
    pthread_mutex_lock(&topic_mutex);
    printf("LIST_OF_TOPICS:\n");
    printf("----------------\n");

    if (list->size == 0) {
        printf("NO_TOPICS_FOUND\n");
    } else {
        topicNode* current = list->head;
        int i = 1;

        while (current != NULL) {
            printf("TOPIC %d:\n", i);
            printf("NAME: %s\n", current->topic.name);
            printf("MULTICAST_ADDRESS: %s\n", current->topic.multicast_address);
            printf("SUBSCRIBED_CLIENTS:\n");

            for (int j = 0; j < current->topic.num_subscribed_clients; j++) {
                char addr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(current->topic.subscribed_clients[j].sin_addr), addr, INET_ADDRSTRLEN);
                printf("%s:%d\n", addr, ntohs(current->topic.subscribed_clients[j].sin_port));
            }

            printf("\n");
            current = current->next;
            i++;
        }
    }
    pthread_mutex_unlock(&topic_mutex);
}

void subscribeTopic(topicList *list_top, client_struct *client, char* name, int sockfd) {

    pthread_mutex_lock(&topic_mutex);
    topic_struct *topic = getTopic(list_top, name);
    if (topic == NULL) {
        // Topic not found
        printf("TOPIC '%s' NOT_FOUND\n", name);
        return;
    }
    // Add client to the topic's list of subscribed clients
    if (topic->num_subscribed_clients >= MAX_CLIENTS) {
        printf("TOPIC %s HAS MAXIMUM NUMBER OF SUBSCRIBERS\n", topic->name);
        return;
    }
    topic->subscribed_clients[topic->num_subscribed_clients] = client->address;
    topic->num_subscribed_clients++;
    pthread_mutex_unlock(&topic_mutex);


    pthread_mutex_lock(&client_mutex);
    // Add the topic to the client's list of subscribed topics
    if (client->num_subscribed_topics >= MAX_TOPICS) {
        printf("CLIENT SUBSCRIBED TO MAXIMUM NUMBERT OF TOPICS\n");
        return;
    }
    strcpy(client->topics[client->num_subscribed_topics], name);
    client->num_subscribed_topics++;
    // Add the topic's multicast address to the client's list of multicast addresses
    if (inet_aton(topic->multicast_address, &client->multicast_addresses[client->num_subscribed_topics-1][0]) == 0) {
        perror("ERROR_CONVERTING MULTICAST_ADDRESS_TO_NETWORK_FORMAT\n");
        return;
    }
    pthread_mutex_unlock(&client_mutex);

    // Send the multicast address to the client
    if (send(sockfd, topic->multicast_address, strlen(topic->multicast_address), 0) == -1) {
        perror("ERROR -> SENDING_MULTICAST_ADDRESS_TO_CLIENT\n");
        return;
    }
}




