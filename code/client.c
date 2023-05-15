#include "client.h"

int main(int argc, char *argv[]) {

    // Initialization code here
    if (argc != 2) {
        printf("Usage: {Endereço_Servidor} {PORTO_NOTICIAS}\n");
        exit(1);
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
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        erro("ERROR opening socket");
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(porto_noticias);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        erro("ERROR connecting to server");
    }

    while (1) {
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
            if (send(sockfd, buffer, MAXLINE, 0) == -1) {
                perror("Error sending message to server\n");
                exit(EXIT_FAILURE);
            }
            char recv_buffer[MAXLINE];
            if (recv(sockfd, recv_buffer, MAXLINE, 0) == -1) {
                perror("Error receiving command from server");
                exit(EXIT_FAILURE);
            }

            char *multicast_address = strtok(recv_buffer, " \n");
            HandleMulticastArgs args = {multicast_port,multicast_address};

            for(int i = 0; i < MAX_TOPICS; i++){
                for(int j = 0; j < 2; j++){
                    if(topic_threads[i][j] == 0){
                        if(j==0){
                            if (pthread_create(&topic_threads[i][j], NULL, multicast_sender_handler, &args) != 0) {
                                erro("ERROR creating multicast thread");
                            }
                        }
                        else{
                            if (pthread_create(&topic_threads[i][j], NULL, multicast_receiver_handler, &args) != 0) {
                                erro("ERROR creating multicast thread");
                            }
                        }
                    }
                }
                break;
            }
            multicast_port++;
        }
        else if(strcmp(token,"LIST_TOPICS") == 0){
            if (send(sockfd, buffer, MAXLINE, 0) == -1) {
                perror("Error sending message to server\n");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(token,"CREATE_TOPIC") == 0){
             if (send(sockfd, buffer, MAXLINE, 0) == -1) {
                perror("Error sending message to server\n");
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(token,"SEND_NEWS") == 0){
            if (send(sockfd, buffer, MAXLINE, 0) == -1) {
                perror("Error sending message to server\n");
                exit(EXIT_FAILURE);
            }
        }
        else{
            printf("Invalid Command\n");
        }
    }

    close(sockfd);
    pthread_cancel(multicast_thread);
    pthread_exit(NULL);
}

// TO COMPLETE
void *multicast_sender_handler(void *arg) {
    HandleMulticastArgs* args = (HandleMulticastArgs*) arg;
    int port = args->multicast_port;
    char *address = args->multicast_address;

    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        pthread_exit(NULL);
    }

    // Set up the multicast group information
    struct sockaddr_in multicast_addr;
    bzero((char *)&multicast_addr, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_port = htons(port);
    multicast_addr.sin_addr.s_addr = inet_addr(address);

    while(1){
        char recv_buffer[MAXLINE];
        if (recv(sockfd, recv_buffer, MAXLINE, 0) == -1) {
            perror("Error receiving command from server");
            exit(EXIT_FAILURE);
        }
        char *token= strtok(recv_buffer, " \n");

        if (strcmp(token,"VALIDATION SUCCESSFUL") == 0){
            char message[MAXLINE];
            printf("SENDER_PROMPT: ");
            fgets(message, MAXLINE, stdin);
            message[strcspn(message, "\n")] = '\0';

            // Send the message to the multicast group
            if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0) {
                perror("Error sending multicast message");
                break;
            }
        }
    }
    
    return NULL;
}

void *multicast_receiver_handler(void *arg){
    HandleMulticastArgs* args = (HandleMulticastArgs*) arg;
    int port = args->multicast_port;
    char *address = args->multicast_address;

    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        pthread_exit(NULL);
    }

    // Set up the multicast group information
    struct sockaddr_in multicast_addr;
    bzero((char *)&multicast_addr, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr(address);
    multicast_addr.sin_port = htons(port);

    // Bind the socket to the multicast address and port
    if (bind(sockfd, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0) {
        perror("Error binding socket");
        close(sockfd);
        pthread_exit(NULL);
    }

    // Join the multicast group
    struct ip_mreq multicast_req;
    multicast_req.imr_multiaddr.s_addr = inet_addr(address);
    multicast_req.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multicast_req, sizeof(multicast_req)) < 0) {
        perror("Error joining multicast group");
        close(sockfd);
        pthread_exit(NULL);
    }

    // Receive multicast messages
    char message[MAXLINE];
    while (1) {
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);
        ssize_t num_bytes = recvfrom(sockfd, message, MAXLINE, 0, (struct sockaddr *)&sender_addr, &sender_len);
        if (num_bytes < 0) {
            perror("Error receiving multicast message");
            break;
        }
        message[num_bytes] = '\0';
        printf("Received: %s\n", message);
        memset(message,0,MAXLINE);
    }

    return NULL;
}

void cleanup(int sig){
    printf("SYSTEM EXITING\n");
    
    for (int i = 0; i < MAX_TOPICS; i++) {
        for(int j = 0; j < 2; j++){
            if (topic_threads[i][j] != 0) {
                pthread_cancel(topic_threads[i][j]);
            }
        }
    }
}