#include "server.h"

// Driver code
int main() {

    struct sockaddr_in servaddr_udp, servaddr_tcp, cliaddr_tcp;
    socklen_t len_tcp = sizeof(cliaddr_tcp);
    int udp_sockfd, tcp_sockfd, conn_tcp;

    // Cria um socket para recepção de pacotes UDP
	if((udp_sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		erro("Erro na criação do socket");
	}

    // Filling server information
    servaddr_udp.sin_family = AF_INET;
    servaddr_udp.sin_addr.s_addr = INADDR_ANY;
    servaddr_udp.sin_port = htons(PORT_UDP);

    // Associa o socket à informação de endereço
	if(bind(udp_sockfd,(struct sockaddr*)&servaddr_udp, sizeof(servaddr_udp)) == -1) {
		erro("Erro no bind");
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

    while (1) {

        admin_process = fork();

        if (admin_process == 0) {
            admin_authentication(udp_sockfd);
            exit(0);
        }

        if ((conn_tcp = accept(tcp_sockfd, (struct sockaddr *)&cliaddr_tcp, &len_tcp)) == -1) {
            erro("Erro ao aceitar conexão TCP");
        }

        client_process = fork();

        if (client_process == 0) {
            client_authentication(conn_tcp);
            close(conn_tcp);
            exit(0);
        }

    }

    return 0;

}