#include "server.h"

// Driver code
int main(int argc, char *argv[]) {

    signal(SIGINT, SIG_IGN);

    if (argc != 4) {
        printf("Usage: %s {PORTO_NOTICIAS} {PORTO_CONFIG} {ficheiro configuração}\n",argv[0]);
        exit(1);
    }

    int PORTO_NOTICIAS = atoi(argv[1]);
    int PORTO_CONFIG = atoi(argv[2]);
    char *file_config = argv[3];

    list_clients = createClientList();
    list_topics = newTopicList();

    struct sockaddr_in servaddr_udp, servaddr_tcp, cliaddr_tcp;
    socklen_t len_tcp = sizeof(cliaddr_tcp);
    int udp_sockfd, tcp_sockfd, conn_tcp, j;


    if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("Error creating UDP socket");
        exit(EXIT_FAILURE);
    }

    // Fill server information
    servaddr_udp.sin_family = AF_INET;
    servaddr_udp.sin_addr.s_addr = INADDR_ANY;
    servaddr_udp.sin_port = htons(PORTO_CONFIG);

    // Bind UDP socket to address
    if (bind(udp_sockfd, (struct sockaddr*)&servaddr_udp, sizeof(servaddr_udp)) == -1) {
        perror("Error binding UDP socket");
        exit(EXIT_FAILURE);
    }

    HandleAdminArgs admin_args = {udp_sockfd, list_clients, list_topics, file_config};

    // Start admin authentication thread
    if (pthread_create(&admin_thread, NULL, handle_admin, &admin_args) != 0) {
        perror("Error creating admin thread");
        exit(EXIT_FAILURE);
    }

     // Create TCP socket
    if ((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        erro("Erro na criação do socket TCP");
    }

    // Filling server information
    servaddr_tcp.sin_family = AF_INET;
    servaddr_tcp.sin_addr.s_addr = INADDR_ANY;
    servaddr_tcp.sin_port = htons(PORTO_NOTICIAS);

     // Bind TCP socket to address
    if (bind(tcp_sockfd, (struct sockaddr*)&servaddr_tcp, sizeof(servaddr_tcp)) == -1) {
        erro("Erro no bind do socket TCP");
    }

      // Listen for incoming TCP connections
    if (listen(tcp_sockfd, SOMAXCONN) == -1) {
        erro("Erro no listen do socket");
    }

    signal(SIGINT, cleanup);

    // Handle client connections
    while (1) {

        if ((conn_tcp = accept(tcp_sockfd, (struct sockaddr *)&cliaddr_tcp, &len_tcp)) == -1) {
            perror("Error accepting TCP connection");
            continue;
        }

        HandleClientArgs client_args = {conn_tcp, list_clients, list_topics, file_config};

        // Find an available slot for the client thread
        for (j = 0; j < MAX_CLIENTS; j++) {
            if (client_threads[j] == 0) {
                if (pthread_create(&client_threads[j], NULL, handle_client, &client_args) != 0) {
                    perror("error creating client thread");
                    close(conn_tcp);
                    break;
                }
                break;
            }
        }

        // Maximum number of clients reached
        if (j == MAX_CLIENTS) {
            fprintf(stderr, "Maximum number of clients reached.\n");
            close(conn_tcp);
        }
    }

    wait(NULL);

    return 0;
}

// Function to handle admin authentication
void *handle_admin(void *arg) {
    HandleAdminArgs* args = (HandleAdminArgs*) arg;
    int udp_sockfd = args->udp_sockfd;
    clientList* list = args->list;
    topicList * list_top = args->list_top;
    char* file = args->file_config;
    admin_authentication(udp_sockfd,list,list_top,file);
    return NULL;
}

// Function to handle client authentication
void *handle_client(void *arg) {
    HandleClientArgs* args = (HandleClientArgs*) arg;
    int conn_tcp = args->conn_tcp;
    clientList* list = args->list;
    topicList * list_top = args->list_top;
    char* file = args->file_config;
    client_authentication(conn_tcp, list, list_top,file);
    close(conn_tcp);
    return NULL;
}

void cleanup(int sig){
    printf("SYSTEM EXITING\n");
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_threads[i] != 0) {
            pthread_cancel(client_threads[i]);
        }
    }

    pthread_cancel(admin_thread);

    destroyClientList(list_clients);
    destroyTopicList(list_topics); 

    exit(0);
}