#include "server.h"

//TO-DO FINISHED

// Driver code
int main(int argc, char *argv[]) {

    signal(SIGINT, SIG_IGN);

    if (argc != 4) {
        printf("USAGE: %s {PORTO_NOTICIAS} {PORTO_CONFIG} {CONFIG_FILE}\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    int PORTO_NOTICIAS = atoi(argv[1]);
    int PORTO_CONFIG = atoi(argv[2]);
    char *file_config = argv[3];

    init_shm();

    list_clients = createClientList();
    list_topics = newTopicList();

    struct sockaddr_in servaddr_udp, servaddr_tcp, cliaddr_tcp;
    socklen_t len_tcp = sizeof(cliaddr_tcp);
    int udp_sockfd, tcp_sockfd, conn_tcp;


    if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("ERROR CREATING UDP SOCKET\n");
        exit(EXIT_FAILURE);
    }

    // Fill server information
    servaddr_udp.sin_family = AF_INET;
    servaddr_udp.sin_addr.s_addr = INADDR_ANY;
    servaddr_udp.sin_port = htons(PORTO_CONFIG);

    // Bind UDP socket to address
    if (bind(udp_sockfd, (struct sockaddr*)&servaddr_udp, sizeof(servaddr_udp)) == -1) {
        perror("ERROR BINDING UDP SOCKET\n");
        exit(EXIT_FAILURE);
    }

    HandleAdminArgs admin_args = {udp_sockfd, list_clients, list_topics, file_config};

    // Start admin authentication thread
    if (pthread_create(&admin_thread, NULL, handle_admin, &admin_args) != 0) {
        perror("ERROR CREATING ADMIN THREAD\n");
        exit(EXIT_FAILURE);
    }

     // Create TCP socket
    if ((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("ERROR CREATING TCP SOCKET\n");
        exit(EXIT_FAILURE);
    }

    // Filling server information
    servaddr_tcp.sin_family = AF_INET;
    servaddr_tcp.sin_addr.s_addr = INADDR_ANY;
    servaddr_tcp.sin_port = htons(PORTO_NOTICIAS);

     // Bind TCP socket to address
    if (bind(tcp_sockfd, (struct sockaddr*)&servaddr_tcp, sizeof(servaddr_tcp)) == -1) {
        perror("ERROR BINDING TCP SOCKET\n");
        exit(EXIT_FAILURE);
    }

      // Listen for incoming TCP connections
    if (listen(tcp_sockfd, SOMAXCONN) == -1) {
        perror("ERROR LISTENING SOCKET\n");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, cleanup);

    // Handle client connections
    while (1) {

        if ((conn_tcp = accept(tcp_sockfd, (struct sockaddr *)&cliaddr_tcp, &len_tcp)) == -1) {
            perror("ERROR ACCEPTING TCP CONNECTION\n");
            continue;
        }

        HandleClientArgs client_args = {conn_tcp, list_clients, list_topics, file_config};

        // Find an available slot for the client thread
        for (int j = 0; j < MAX_CLIENTS; j++) {
            if(j < MAX_CLIENTS){
                if (client_threads[j] == 0) {
                    if (pthread_create(&client_threads[j], NULL, handle_client, &client_args) != 0) {
                        perror("ERROR CREATING CLIENT THREAD\n");
                        close(conn_tcp);
                        continue;
                    }
                    break;
                }
            }
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

void init_shm(){
    key_t key = ftok("/path/to/file", 'R');
    
    if ((shm_id = shmget(key, sizeof(struct Entry) * MAX_TOPICS, IPC_CREAT | IPC_EXCL | 0700)) < 1){
        printf("ERROR IN SHMGET WITH IPC_CREAT\n");
        exit(EXIT_FAILURE);
    }

    if((dictionary = (struct Entry*) shmat(shm_id, NULL, 0)) == (struct Entry*)-1){
        printf("ERROR ATTACHING SHARED MEMORY\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_TOPICS; i++) {
        dictionary[i] = (struct Entry){
            .address = "",
            .port = 0
        };
    };
}

void cleanup(int sig){
    printf("SYSTEM EXITING\n");
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_threads[i] != 0) {
            pthread_cancel(client_threads[i]);
        }
    }

    pthread_cancel(admin_thread);

    if(shmdt(dictionary) == -1)printf("ERROR DETACHING SHM SEGMENT\n");
    if(shmctl(shm_id, IPC_RMID, NULL) == -1)printf("ERROR REMOVING SHM SEGMENT\n");
    destroyClientList(list_clients);
    destroyTopicList(list_topics); 

    exit(EXIT_SUCCESS);
}