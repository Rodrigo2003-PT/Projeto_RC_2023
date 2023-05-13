#include "admin_functions.h"

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_admin_console(int sockfd, ClientList *list, TopicList *list_top){

    int recv_len;
    char buffer[MAXLINE];
    struct sockaddr_in cliaddr;
    socklen_t slen = sizeof(cliaddr);

    sem_init(&file_semaphore, 0, 1);

    while (1) {

        // Receive command from admin console
        if((recv_len = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr *) &cliaddr, (socklen_t *)&slen)) == -1) {
            erro("Erro no recvfrom");
        }
        buffer[recv_len] = '\0';

        // Parse the command
        char* token = strtok(buffer, " ");

        if (strcmp(token, "ADD_USER") == 0) {
            char* username = strtok(NULL, " ");
            char* password = strtok(NULL, " ");
            char* userType = strtok(NULL, " \n");

            sem_wait(&file_semaphore);

            if (addUser(username, password, userType)) {
                sendto(sockfd, "user added successfully\n", strlen("user added successfully\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            } else {
                sendto(sockfd, "failed to add user\n", strlen("failed to add user\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
                }
            
            sem_post(&file_semaphore);
        } 
        else if (strcmp(token, "DEL") == 0) {

            char* username = strtok(NULL, " \n");

            sem_wait(&file_semaphore);

            if (deleteUser(username)) {
                removeClient(list,list_top,username);
                writeClientListToFile(list);
                writeTopicListToFile(list_top);
                sendto(sockfd, "user deleted successfully\n", strlen("user deleted successfully\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            }
            else {
                sendto(sockfd, "failed to delete user\n", strlen("failed to delete user\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            }

            sem_post(&file_semaphore);
        } 
        else if (strcmp(token, "LIST\n") == 0) {
            sem_wait(&file_semaphore);
            listUsers(sockfd,slen,cliaddr);
            sem_post(&file_semaphore);
        }
        else if (strcmp(token, "QUIT\n") == 0) {
            quitConsole(sockfd);
            break;
        } 
        else if (strcmp(token, "QUIT_SERVER\n") == 0) {
            quitServer(sockfd);
        }
        else {
            sendto(sockfd, "invalid command\n", strlen("invalid command\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
        }
    }
}

void handle_leitor_commands(int sockfd, ClientList *list, TopicList *list_top, Client *client) {

    char* token;
    char buffer[MAXLINE];

    while (1) {
        // Receive command from client
        if (recv(sockfd, buffer, MAXLINE, 0) == -1) {
            perror("Error receiving command from client");
            exit(EXIT_FAILURE);
        }

        // Parse command
        token = strtok(buffer, " \n");

        if (strcmp(token, "LIST_TOPICS") == 0) {
            printTopics(list_top);
        } 
        else if (strcmp(token, "SUBSCRIBE_TOPIC") == 0) {
            // Parse topic_id
            token = strtok(NULL, " \n");
            // Subscribe to topic
            subscribe_topic(list_top,client,token);
        } 
        else {
            // Invalid command
            if (send(sockfd, "Invalid command\n", 17, 0) == -1) {
                perror("Error sending message to client");
                exit(EXIT_FAILURE);
            }
        }
    }
}

//TO-DO -> INTEGRAL IMPLEMENTATION
void handle_jornalista_commands(int sockfd, ClientList *list, TopicList *list_top, Client *client){
    return;
}

void admin_authentication(int sockfd, ClientList *list, TopicList *list_top){

    int recv_len;
    char buffer[MAXLINE];
    bool authenticated = false;
    struct sockaddr_in cliaddr;
    socklen_t slen = sizeof(cliaddr);

    while (!authenticated) {

        // Receive credentials from admin console
        if((recv_len = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr *) &cliaddr, (socklen_t *)&slen)) == -1) {
            erro("Erro no recvfrom");
        }
        
        // Para ignorar o restante conteúdo (anterior do buffer)
	    buffer[recv_len]='\0';

        // Parse the credentials
        char* username = strtok(buffer, " ");
        char* password = strtok(NULL, " \n");

        // Verify credentials
        if (strcmp(username, "admin") == 0 && strcmp(password, "password") == 0) {
            authenticated = true;
            sendto(sockfd, "authentication successful\n", strlen("authentication successful\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            handle_admin_console(sockfd, list, list_top);
        } 
        else {
            sendto(sockfd, "authentication failed: <username> <password>\n", strlen("authentication failed <username> <password>\n"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, slen);
            printf("Authentication failed.\n");
        }
    }
}

void client_authentication(int sockfd, ClientList *list, TopicList *list_top){

    char* token;
    int recv_len;
    Client client;
    char buffer[MAXLINE];
    bool authenticated = false;
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

     while (!authenticated) {

        // Receive credentials from client
        if((recv_len = recv(sockfd, buffer, MAXLINE, 0)) == -1) {
            erro("Erro no recv");
        }
        
        // Para ignorar o restante conteúdo (anterior do buffer)
	    buffer[recv_len]='\0';

        // Parse the credentials
        char* username = strtok(buffer, " ");
        char* password = strtok(NULL, " \n");

        // Verify credentials
        if ((token = authenticate_client(username,password)) != NULL) {
            authenticated = true;
            send(sockfd, "authentication successful\n", strlen("authentication successful\n"), 0);

            if(getpeername(sockfd, (struct sockaddr *)&client_address, &client_address_len) == -1){
                printf("Error getting client address\n");
                close(sockfd);
                return;
            }

            client = getClientByName(client_address, username, list);

            if (strcmp(token, "leitor") == 0) {
                handle_leitor_commands(sockfd, list, list_top, &client);
            } 
            else if (strcmp(token, "jornalista") == 0) {
                handle_jornalista_commands(sockfd,list, list_top, &client);
            }
        } 
        else {
            send(sockfd, "authentication failed: <username> <password>\n", strlen("authentication failed <username> <password>\n"),0);
            printf("Authentication failed.\n");
        }
    }
};

char* authenticate_client(char *username, char *password) {

    char line[MAXLINE];
    char *token;
    FILE *fp;

    // Open file for reading
    fp = fopen("users.txt", "r");
    if (fp == NULL) {
        perror("Error opening file");
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
        token = strtok(NULL, ";");

        fclose(fp);
        return token; // Authentication successful
    }

    fclose(fp);
    return NULL; // Authentication failed
}

// Function to handle ADD_USER command
bool addUser(char* username, char* password, char* userType) {

    FILE *fp;
    char line[MAXLINE];
    char* token;
    bool userExists = false;
    
    // Open the file in read mode
    fp = fopen("users.txt", "r");

    // Check if the file was opened successfully
    if (fp == NULL) {
        printf("Error opening file.");
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
        printf("User already exists.\n");
        return false;
    }

    // Open the file in append mode
    fp = fopen("users.txt", "a");

    // Check if the file was opened successfully
    if (fp == NULL) {
        printf("Error opening file.");
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
bool deleteUser(char* username) {

    char tempFile[] = "temp.txt";
    char file[] = "users.txt";
    char buffer[256];

    FILE *fp = fopen(file, "r");
    FILE *temp = fopen(tempFile, "w");

    // Check if file could be opened
    if (fp == NULL || temp == NULL) {
        printf("Error opening file.\n");
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
        printf("User deleted successfully.\n");
        return true;
    } else {
        printf("User not found.\n");
        return false;
    }
}

// Function to handle LIST command
// Can be done iterating through client_list
void listUsers(int sockfd, int slen, struct sockaddr_in cliaddr) {

    FILE* fp;
    char* token;
    char line[MAXLINE];

    fp = fopen("users.txt", "r");

    if (fp == NULL) {
        printf("Error: could not open file\n");
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
    printf("Admin console session ended.\n");
    sem_destroy(&file_semaphore);
    close(sockfd);
}

// Function to handle QUIT_SERVER command
void quitServer(int sockfd) {
    printf("Server shutting down.\n");
    sem_destroy(&file_semaphore);
    exit(EXIT_SUCCESS);
    close(sockfd);
}

void erro(char *s) {
	perror(s);
	exit(1);
}

// Function to create a new client node
ClientNode* createClientNode(Client client) {
    ClientNode* node = (ClientNode*) malloc(sizeof(ClientNode));
    if (node == NULL) {
        fprintf(stderr, "Error: could not allocate memory for client node\n");
        exit(EXIT_FAILURE);
    }
    node->client = client;
    node->next = NULL;
    return node;
}


// Function to create a new client list
ClientList* createClientList() {
    ClientList* list = (ClientList*) malloc(sizeof(ClientList));
    if (list == NULL) {
        fprintf(stderr, "Error: could not allocate memory for client list\n");
        exit(EXIT_FAILURE);
    }
    list->head = NULL;
    list->size = 0;
    return list;
}


void addClient(ClientList* list, Client client) {
    ClientNode* new_node = createClientNode(client);

    if (list->head == NULL) {
        // list is empty
        list->head = new_node;
    } else {
        // add new node to the end of the list
        ClientNode* current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }

    // increment the size of the list
    list->size++;
}


void removeClient(ClientList* client_list, TopicList* topic_list, char *username) {
    // Remove client from client list
    ClientNode* curr_client = client_list->head;
    ClientNode* prev_client = NULL;

    struct sockaddr_in address = get_client_address_by_username(client_list, username);

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

    // Remove client from topic subscriber lists
    TopicNode* curr_topic = topic_list->head;
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
}

struct sockaddr_in get_client_address_by_username(ClientList* client_list, char* username) {
    ClientNode* curr_client = client_list->head;
    while (curr_client != NULL) {
        if (strcmp(curr_client->client.user_name, username) == 0) {
            return curr_client->client.address;
        }
        curr_client = curr_client->next;
    }
    struct sockaddr_in empty_address = {0};
    return empty_address;
}

void destroyClientList(ClientList* list) {
    ClientNode* current = list->head;
    while (current != NULL) {
        ClientNode* temp = current;
        current = current->next;
        free(temp);
    }
    list->head = NULL;
    list->size = 0;
}

void writeClientListToFile(ClientList* list) {
    FILE* file = fopen("clients.dat", "wb");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    ClientNode* current = list->head;
    while (current != NULL) {
        fwrite(&current->client, sizeof(Client), 1, file);
        current = current->next;
    }

    fclose(file);
}

ClientList* readClientsFromFile() {
 
    Client tempClient;
    ClientList* list = createClientList();
    FILE* file = fopen("clients.dat", "rb");

    if (file == NULL) {
        return list;
    }

    // Check if the file is empty
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize == 0) {
        fclose(file);
        return list;
    }

    // Reset file position to beginning
    fseek(file, 0, SEEK_SET);

    while (fread(&tempClient, sizeof(Client), 1, file) == 1) {
        ClientNode* newNode = (ClientNode*)malloc(sizeof(ClientNode));
        if (newNode == NULL) {
            perror("Error allocating memory");
            exit(EXIT_FAILURE);
        }

        // Copy the client information to the new node
        newNode->client = tempClient;

        // Add the new node to the end of the linked list
        newNode->next = NULL;

        if (list->head == NULL) {
            list->head = newNode;
        } 
        else{
            ClientNode* current = list->head;
            while (current->next != NULL)
                current = current->next;
            current->next = newNode;
        }
    }

    fclose(file);
    return list;
}

Client getClientByName(struct sockaddr_in address, char *username, ClientList* list) {
    FILE* file = fopen("clients.dat", "rb");

    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    Client client;
    while (fread(&client, sizeof(Client), 1, file)) {
        if (memcmp(&address, &client.address, sizeof(struct sockaddr_in)) == 0) {
            fclose(file);
            return client;
        }
    }

    fclose(file);
    // If no matching client is found, return a default initialized client with address value set to address and other fields set to default values
    Client notFoundClient = {0};
    strcpy(notFoundClient.user_name, username);
    notFoundClient.address = address;
    notFoundClient.num_subscribed_topics = 0;
    memset(notFoundClient.topics, 0, sizeof(notFoundClient.topics));
    memset(notFoundClient.multicast_addresses, 0, sizeof(notFoundClient.multicast_addresses));
    pthread_mutex_lock(&clients_mutex);
    addClient(list, notFoundClient);
    writeClientListToFile(list);
    pthread_mutex_unlock(&clients_mutex);

    return notFoundClient;
}

void printClientList(ClientList* list) {
    ClientNode* current = list->head;

    printf("Client List:\n");
    printf("------------\n");
    while (current != NULL) {
        printf("Username: %s\n", current->client.user_name);
        printf("Number of subscribed topics: %d\n", current->client.num_subscribed_topics);
        printf("Subscribed topics: ");
        for (int i = 0; i < current->client.num_subscribed_topics; i++) {
            printf("%s ", current->client.topics[i]);
        }
        printf("\n");
        printf("Address: %s\n", inet_ntoa(current->client.address.sin_addr));
        printf("Multicast addresses: ");
        for (int i = 0; i < current->client.num_subscribed_topics; i++) {
            char multicast_str[INET_ADDRSTRLEN];
            for (int j = 0; j < 16; j++) {
                if (inet_ntop(AF_INET, &(current->client.multicast_addresses[i][j]), multicast_str, INET_ADDRSTRLEN) == NULL) {
                    perror("Error converting multicast address to string");
                    exit(EXIT_FAILURE);
                }
                printf("%s ", multicast_str);
            }
        }
        printf("\n\n");
        current = current->next;
    }
}

TopicList* newTopicList() {
    TopicList* topicList = (TopicList*) malloc(sizeof(TopicList));
    topicList->head = NULL;
    topicList->size = 0;
    return topicList;
}

void destroyTopicList(TopicList* topicList) {
    if (topicList == NULL) {
        return;
    }

    TopicNode* currentNode = topicList->head;
    TopicNode* nextNode;

    while (currentNode != NULL) {
        nextNode = currentNode->next;
        free(currentNode);
        currentNode = nextNode;
    }

    free(topicList);
}


void addTopic(TopicList* topicList, Topic newTopic) {
    TopicNode* newNode = (TopicNode*) malloc(sizeof(TopicNode));
    newNode->topic = newTopic;
    newNode->next = NULL;

    if (topicList->head == NULL) {
        topicList->head = newNode;
    } else {
        TopicNode* currentNode = topicList->head;
        while (currentNode->next != NULL) {
            currentNode = currentNode->next;
        }
        currentNode->next = newNode;
    }

    topicList->size++;
}


void removeTopic(TopicList* list, char* topic_name) {
    // Check if the list is empty
    if (list->head == NULL) {
        printf("The topic list is empty\n");
        return;
    }
    // Check if the head node is the target node
    if (strcmp(list->head->topic.name, topic_name) == 0) {
        TopicNode* temp = list->head;
        list->head = list->head->next;
        free(temp);
        list->size--;
        return;
    }
    // Traverse the list to find the target node
    TopicNode* current = list->head;
    while (current->next != NULL && strcmp(current->next->topic.name, topic_name) != 0) {
        current = current->next;
    }
    // Check if the target node was found
    if (current->next == NULL) {
        printf("Topic not found\n");
        return;
    }
    // Remove the target node
    TopicNode* temp = current->next;
    current->next = current->next->next;
    free(temp);
    list->size--;
}

void writeTopicListToFile(TopicList* list) {
    FILE* file = fopen("topics.dat", "wb");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    TopicNode* current = list->head;
    while (current != NULL) {
        fwrite(&current->topic, sizeof(Topic), 1, file);
        current = current->next;
    }

    fclose(file);
}

TopicList* readTopicsFromFile() {
    Topic tempTopic;
    TopicList* list = newTopicList();
    FILE* file = fopen("topics.dat", "rb");

    if (file == NULL) {
        return list;
    }

    // Check if the file is empty
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize == 0) {
        fclose(file);
        return list;
    }

    // Reset file position to beginning
    fseek(file, 0, SEEK_SET);

    while (fread(&tempTopic, sizeof(Topic), 1, file) == 1) {
        TopicNode* newNode = (TopicNode*)malloc(sizeof(TopicNode));
        if (newNode == NULL) {
            perror("Error allocating memory");
            exit(EXIT_FAILURE);
        }

        // Copy the topic information to the new node
        newNode->topic = tempTopic;

        // Add the new node to the end of the linked list
        newNode->next = NULL;

        if (list->head == NULL) {
            list->head = newNode;
        } else {
            TopicNode* current = list->head;
            while (current->next != NULL)
                current = current->next;
            current->next = newNode;
        }
    }

    fclose(file);
    return list;
}


Topic* getTopicByName(TopicList* topicList, const char* name) {
    TopicNode* currentNode = topicList->head;
    while (currentNode != NULL) {
        if (strcmp(currentNode->topic.name, name) == 0) {
            return &(currentNode->topic);
        }
        currentNode = currentNode->next;
    }
    return NULL;
}

void printTopics(TopicList* list) {
    printf("List of topics:\n");
    printf("----------------\n");

    if (list->size == 0) {
        printf("No topics found.\n");
    } else {
        TopicNode* current = list->head;
        int i = 1;

        while (current != NULL) {
            printf("Topic %d:\n", i);
            printf("Name: %s\n", current->topic.name);
            printf("Multicast Address: %s\n", current->topic.multicast_address);
            printf("Subscribed Clients:\n");

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
}

void subscribe_topic(TopicList *list_top, Client *client, char* name) {

    Topic *topic = getTopicByName(list_top, name);

    if (topic == NULL) {
        // Topic not found
        printf("Topic '%s' not found\n", name);
        return;
    }
    
    // Add client to the topic's list of subscribed clients
    if (topic->num_subscribed_clients >= MAX_CLIENTS) {
        printf("Topic %s already has maximum number of subscribers.\n", topic->name);
        return;
    }
    topic->subscribed_clients[topic->num_subscribed_clients] = client->address;
    topic->num_subscribed_clients++;
    
    // Add the topic to the client's list of subscribed topics
    if (client->num_subscribed_topics >= MAX_TOPICS) {
        printf("Client already subscribed to maximum number of topics.\n");
        return;
    }
    strcpy(client->topics[client->num_subscribed_topics], name);
    client->num_subscribed_topics++;
    
    // Add the topic's multicast address to the client's list of multicast addresses
    if (inet_aton(topic->multicast_address, &client->multicast_addresses[client->num_subscribed_topics-1][0]) == 0) {
        perror("Error converting multicast address to network format");
        return;
    }
}




