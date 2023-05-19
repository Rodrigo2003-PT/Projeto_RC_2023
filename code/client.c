#include "client.h"

bool auth = false;
bool is_reused = false;

int main(int argc, char *argv[]) {

    // Initialization code here
    if (argc != 3) {
        printf("Usage: %s {Endereço_Servidor} {PORTO_NOTICIAS}\n",argv[0]);
        exit(1);
    }

    key_t key = ftok("/path/to/file", 'R');
    int shmid = shmget(key, 0, 0);

    if((dictionary = (struct Entry*) shmat(shmid, NULL, 0)) == (struct Entry*)-1){
        printf("ERROR ATTACHING SHARED MEMORY\n");
        exit(0);
    }

    server_address = argv[1];
    multicast_port = 9000;
    porto_noticias = atoi(argv[2]);

    // Create Server_Thread
    if (pthread_create(&server_thread, NULL, server_handler, NULL) < 0) {
        erro("ERROR creating server thread");
    }

    signal(SIGINT, cleanup);

    pthread_join(server_thread, NULL);

    return 0;
}

void *server_handler(void *arg) {
    int sockfd;
    char buffer[MAXLINE];
    char recv_buffer[MAXLINE];
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        erro("ERROR opening socket");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(porto_noticias);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        erro("ERROR connecting to server");
        close(sockfd);
        exit(1);
    }

    while(!auth){
        bzero(recv_buffer,MAXLINE);
        bzero(buffer, MAXLINE);
        fgets(buffer, MAXLINE - 1, stdin);
        
        buffer[strcspn(buffer, "\n")] = 0;
        if (send(sockfd, buffer, MAXLINE, 0) == -1) {
            perror("Error sending message to server\n");
            exit(EXIT_FAILURE);
        }
        if (recv(sockfd, recv_buffer, MAXLINE, 0) == -1) {
            perror("Error receiving command from server");
            exit(EXIT_FAILURE);
        }
        char *token= strtok(recv_buffer, " \n");

        if (strcmp(token,"authentication_successful") == 0){
            printf("%s\n",token);
            auth = true;
        }
    }

    while (1) {
        bzero(recv_buffer,MAXLINE);
        bzero(buffer, MAXLINE);
        fgets(buffer, MAXLINE - 1, stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        char buffer_copy[MAXLINE];
        strcpy(buffer_copy, buffer);
        char* token = strtok(buffer_copy, " \n");

        if (strcmp(token, "QUIT") == 0) {
            break;
        }
        else if(strcmp(token,"SUBSCRIBE_TOPIC") == 0){
            is_reused = false;
            if (send(sockfd, buffer, MAXLINE, 0) == -1) {
                perror("Error sending message to server\n");
                exit(EXIT_FAILURE);
            }
            if (recv(sockfd, recv_buffer, MAXLINE, 0) == -1) {
                perror("Error receiving command from server");
                exit(EXIT_FAILURE);
            }

            char *multicast_address = strtok(recv_buffer, " \n");
            HandleMulticastArgs args;

            for (int i = 0; i < MAX_TOPICS; i++) {
                if (strcmp(dictionary[i].address,multicast_address) == 0) {
                    strncpy(args.multicast_address, multicast_address, sizeof(args.multicast_address));
                    args.multicast_port = dictionary[i].port;
                    is_reused = true;
                }
            }

            if(!is_reused){
                strncpy(args.multicast_address, multicast_address, sizeof(args.multicast_address));
                args.multicast_port = multicast_port;
            }

            for(int i = 0; i < MAX_TOPICS; i++){
                if(topic_threads[i] == 0){
                    if (pthread_create(&topic_threads[i], NULL, multicast_receiver_handler, &args) != 0) {
                        erro("ERROR creating multicast thread");
                    }
                    break;
                }
            }
            multicast_port++;
        }

        else if(strcmp(token,"LIST_TOPICS") == 0){
            if (send(sockfd, buffer, MAXLINE, 0) == -1) {
                perror("Error sending message to server\n");
                exit(EXIT_FAILURE);
            }
            if (recv(sockfd, recv_buffer, MAXLINE, 0) == -1) {
                perror("Error receiving command from server");
                exit(EXIT_FAILURE);
            }
            printf("%s",recv_buffer);
        }
        else if(strcmp(token,"CREATE_TOPIC") == 0){
             if (send(sockfd, buffer, MAXLINE, 0) == -1) {
                perror("Error sending message to server\n");
                exit(EXIT_FAILURE);
            }
            if (recv(sockfd, recv_buffer, MAXLINE, 0) == -1) {
                perror("Error receiving command from server");
                exit(EXIT_FAILURE);
            }
            printf("%s\n",recv_buffer);
        }
        else if(strcmp(token,"SEND_NEWS") == 0){
            char* multicast_id = strtok(NULL," ");
            char* message = strtok(NULL,"\n");
           
            if (send(sockfd, buffer, MAXLINE, 0) == -1) {
                perror("Error sending message to server\n");
                exit(EXIT_FAILURE);
            }

            if (recv(sockfd, recv_buffer, MAXLINE, 0) == -1) {
                perror("Error receiving command from server");
                exit(EXIT_FAILURE);
            }

            if(strncmp(recv_buffer,"SUCCESSFULL",11) == 0){
                pthread_mutex_lock(&mutex);
                for (int i = 0; i < MAX_TOPICS; i++) {
                    if (strcmp(dictionary[i].address, multicast_id) == 0) {
                        printf("sockfd:%d port:%d multicast_id:%s message:%s\n",dictionary[i].sockfd,dictionary[i].port,multicast_id, message);
                        multicast_send(dictionary[i].sockfd, dictionary[i].port, multicast_id, message);
                        break;
                    }
                }
                pthread_mutex_unlock(&mutex);
            }  
        }
        else{
            printf("Invalid Command\n");
        }
    }

    close(sockfd);
    pthread_exit(NULL);
}

void multicast_send(int sockfd, int multicast_port, char* group_address, char* message){
    struct sockaddr_in multicast_addr;
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_port = htons(multicast_port); 

    if (inet_aton(group_address, &multicast_addr.sin_addr) == 0) {
        perror("Invalid multicast address");
        exit(EXIT_FAILURE);
    }

    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0) {
        perror("Error sending multicast message");
        exit(EXIT_FAILURE);
    }
}

void *multicast_receiver_handler(void *arg){
    HandleMulticastArgs* args = (HandleMulticastArgs*) arg;
    char group_address[16];
    strncpy(group_address, args->multicast_address, sizeof(group_address));
    int port = args->multicast_port;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        pthread_exit(NULL);
    }

    // Set the SO_REUSEADDR option
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Error setting socket option");
        close(sockfd);
        pthread_exit(NULL);
    }

    // enable multicast on the socket
    int enable = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &enable, sizeof(enable)) < 0) {
        perror("setsockopt");
        pthread_exit(NULL);
    }

    printf("sockfd:%d port:%d\n",sockfd,port);
    
    pthread_mutex_lock(&mutex);
    addEntry(port,sockfd,group_address);
    pthread_mutex_unlock(&mutex);

    // Set up the multicast group information
    struct sockaddr_in multicast_addr;
    bzero((char *)&multicast_addr, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr(group_address);
    multicast_addr.sin_port = htons(port);


    // Bind the socket to the multicast address and port
    if (bind(sockfd, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0) {
        perror("Error binding socket");
        close(sockfd);
        pthread_exit(NULL);
    }

    // Join the multicast group
    struct ip_mreq multicast_req;
    multicast_req.imr_multiaddr.s_addr = inet_addr(group_address);
    multicast_req.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicast_req, sizeof(multicast_req)) < 0) {
        perror("Error joining multicast group");
        close(sockfd);
        pthread_exit(NULL);
    }

    // Receive multicast messages
    char message[MAXLINE];
    ssize_t num_bytes;
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);
    while (1) {
        num_bytes = recvfrom(sockfd, message, MAXLINE, 0, (struct sockaddr *)&sender_addr, &sender_len);
        if (num_bytes < 0) {
            perror("Error receiving multicast message");
            break;
        }
        message[num_bytes] = '\0';
        printf("Received from %s: %s\n", group_address, message);
        memset(message,0,MAXLINE);
    }

close(sockfd);
return NULL;
}

void addEntry(int port, int sockfd, const char* address) {
    // Check if sockfd already exists in the dictionary
    for (int i = 0; i < MAX_TOPICS; i++) {
        if (dictionary[i].sockfd == sockfd) {
            printf("sockfd %d already exists in the dictionary.\n", sockfd);
            return;
        }
    }

    // Add the new entry to the dictionary
    for (int i = 0; i < MAX_TOPICS; i++) {
        if (dictionary[i].sockfd == 0) {
            printf("%d\n", i);
            dictionary[i].port = port;
            dictionary[i].sockfd = sockfd;
            strncpy(dictionary[i].address, address, 16);
            return;
        }
    }

    printf("Dictionary is full. Cannot add new entry.\n");
}

void cleanup(int sig){
    printf("SYSTEM EXITING\n");

    for (int i = 0; i < MAX_TOPICS; i++) {
        if (topic_threads[i] != 0) {
            pthread_cancel(topic_threads[i]);
        }
    }
    exit(0);
}