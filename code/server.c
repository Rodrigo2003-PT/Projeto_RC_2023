#include "server.h"

#define PORT_CONFIG 9090
#define MAXLINE 1024

// Driver code
int main() {

    struct sockaddr_in servaddr, cliaddr;
    socklen_t slen = sizeof(cliaddr);
    char buffer[MAXLINE];
    int sockfd;

    // Cria um socket para recepção de pacotes UDP
	if((sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		erro("Erro na criação do socket");
	}

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT_CONFIG);

    // Associa o socket à informação de endereço
	if(bind(sockfd,(struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
		erro("Erro no bind");
	}

    if(admin_authentication(buffer,sockfd,slen,cliaddr))
        handle_admin_console(buffer,sockfd,slen,cliaddr);

    return 0;

}