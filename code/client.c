#include "client.h"

int main(int argc, char *argv[]) {

    // Initialization code here
    if (argc != 2) {
        printf("Usage: {Endereço_Servidor} {PORTO_NOTICIAS}\n");
        exit(1);
    }

    int sock;
    server_address = argv[1];
    porto_noticias = atoi(argv[2]);

    // Create Server_Thread
    if (pthread_create(&server_thread, NULL, server_handler, NULL) < 0) {
        erro("ERROR creating server thread");
    }

    pthread_join(server_thread, NULL);

    return 0;
}

void *server_handler(void *arg) {
    int sockfd, n;
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
            char recv_buffer[MAXLINE];
            if (recv(sockfd, recv_buffer, MAXLINE, 0) == -1) {
                perror("Error receiving command from server");
                exit(EXIT_FAILURE);
            }
            char *multicast_address = strtok(recv_buffer, " \n");
            for(int i = 0; i < MAX_TOPICS; i++){
                if(topic_threads[i] == 0){
                    if (pthread_create(&topic_threads[i], NULL, multicast_handler, (void *)multicast_address) != 0) {
                        erro("ERROR creating multicast thread");
                    }
                break;
                }
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
void *multicast_handler(void *arg) {
    int sockfd;
    struct sockaddr_in multicast_addr;
    char buffer[MAXLINE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        erro("ERROR opening socket");
    }

    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_addr.s_addr = inet_addr((char *)arg);
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_port = htons(0);

    close(sockfd);
    pthread_exit(NULL);
}