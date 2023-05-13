#include "server.h"

// Driver code
int main() {

    ClientList* list_clients = readClientsFromFile();
    TopicList* list_topics = readTopicsFromFile();

    struct sockaddr_in servaddr_udp, servaddr_tcp, cliaddr_tcp;
    socklen_t len_tcp = sizeof(cliaddr_tcp);
    int udp_sockfds[NUM_ADMIN_THREADS];
    int tcp_sockfd, conn_tcp, udp_port, i;

    for (i = 0, udp_port = FIRST_UDP_PORT; i < NUM_ADMIN_THREADS; i++, udp_port++) {

        if ((udp_sockfds[i] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
            perror("Error creating UDP socket");
            exit(EXIT_FAILURE);
        }

        // Fill server information
        servaddr_udp.sin_family = AF_INET;
        servaddr_udp.sin_addr.s_addr = INADDR_ANY;
        servaddr_udp.sin_port = htons(udp_port);

        // Bind UDP socket to address
        if (bind(udp_sockfds[i], (struct sockaddr*)&servaddr_udp, sizeof(servaddr_udp)) == -1) {
            perror("Error binding UDP socket");
            exit(EXIT_FAILURE);
        }

        HandleAdminArgs admin_args = {udp_sockfds[i],list_clients, list_topics};

        // Start admin authentication thread
        if (pthread_create(&admin_threads[i], NULL, handle_admin, &admin_args) != 0) {
            perror("Error creating admin thread");
            exit(EXIT_FAILURE);
        }
    }

     // Create TCP socket
    if ((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        erro("Erro na criação do socket TCP");
    }

    // Filling server information
    servaddr_tcp.sin_family = AF_INET;
    servaddr_tcp.sin_addr.s_addr = INADDR_ANY;
    servaddr_tcp.sin_port = htons(PORT_TCP);

     // Bind TCP socket to address
    if (bind(tcp_sockfd, (struct sockaddr*)&servaddr_tcp, sizeof(servaddr_tcp)) == -1) {
        erro("Erro no bind do socket TCP");
    }

      // Listen for incoming TCP connections
    if (listen(tcp_sockfd, SOMAXCONN) == -1) {
        erro("Erro no listen do socket");
    }

    // Handle client connections
    while (1) {

        if ((conn_tcp = accept(tcp_sockfd, (struct sockaddr *)&cliaddr_tcp, &len_tcp)) == -1) {
            perror("Error accepting TCP connection");
            continue;
        }

        HandleClientArgs client_args = {conn_tcp, list_clients, list_topics};

        // Find an available slot for the client thread
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_threads[i] == 0) {
                if (pthread_create(&client_threads[i], NULL, handle_client, &client_args) != 0) {
                    perror("error creating client thread");
                    close(conn_tcp);
                    break;
                }
                break;
            }
        }

        // Maximum number of clients reached
        if (i == MAX_CLIENTS) {
            fprintf(stderr, "Maximum number of clients reached.\n");
            close(conn_tcp);
        }
    }

    for (i = 0; i < NUM_ADMIN_THREADS; i++) {
        pthread_join(admin_threads[i], NULL);
    }

    for (i = 0; i < MAX_CLIENTS; i++) {
        pthread_join(client_threads[i], NULL);
    }

    return 0;
}

// Function to handle admin authentication
void *handle_admin(void *arg) {
    HandleAdminArgs* args = (HandleAdminArgs*) arg;
    int udp_sockfd = args->udp_sockfd;
    ClientList* list = args->list;
    TopicList * list_top = args->list_top;
    admin_authentication(udp_sockfd,list,list_top);
    return NULL;
}

// Function to handle client authentication
void *handle_client(void *arg) {
    HandleClientArgs* args = (HandleClientArgs*) arg;
    int conn_tcp = args->conn_tcp;
    ClientList* list = args->list;
    TopicList * list_top = args->list_top;
    client_authentication(conn_tcp, list, list_top);
    close(conn_tcp);
    return NULL;
}