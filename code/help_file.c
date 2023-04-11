#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_CLIENTS 10
#define MAX_TOPICOS 10
#define MAX_BUF_SIZE 1024

// estrutura para armazenar informações dos clientes
typedef struct {
    char nome[20];
    int sock_fd;
    int topico;
} Cliente;

// estrutura para armazenar informações dos tópicos
typedef struct {
    Cliente clientes[MAX_CLIENTS];
    char endereco_multicast[20];
    int num_clientes;
    char nome[20];
} Topico;

// vetor de tópicos
Topico topicos[MAX_TOPICOS];

// função para enviar mensagem a todos os clientes de um tópico
void enviar_mensagem(int sockfd, Topico *topico, char *mensagem) {
    
    memset(&addr, 0, addrlen);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    inet_aton(topico->endereco_multicast, &addr.sin_addr);
    sendto(sock_fd, mensagem, strlen(mensagem), 0, (struct sockaddr *) &addr, addrlen);
    close(sock_fd);
}

// função para adicionar um cliente a um tópico
void adicionar_cliente(Topico *topico, Cliente *cliente) {
    if (topico->num_clientes >= MAX_CLIENTS) {
        printf("Limite de clientes atingido para o tópico %s\n", topico->nome);
        return;
    }
    topico->clientes[topico->num_clientes++] = *cliente;
    cliente->topico = topico - topicos;
    printf("Cliente %s adicionado ao tópico %s\n", cliente->nome, topico->nome);
}

// função para remover um cliente de um tópico
void remover_cliente(Topico *topico, Cliente *cliente) {

    int i;

    for (i = 0; i < topico->num_clientes; i++) {

        if (topico->clientes[i].sock_fd == cliente->sock_fd) {
            topico->num_clientes--;
            topico->clientes[i] = topico->clientes[topico->num_clientes];
            printf("Cliente %s removido do tópico %s\n", cliente->nome, topico->nome);
            return;
        }
    }
}

int main(int argc, char *argv[]) {
int sock_fd, new_sock_fd, port_num, clilen;
struct sockaddr_in serv_addr, cli_addr;

// inicializa o vetor de tópicos
int i;
for (i = 0; i < MAX_TOPICOS; i++) {
    topicos[i].num_clientes = 0;
}

// cria um socket TCP para o servidor
sock_fd = socket(AF_INET, SOCK_STREAM, 0);
if (sock_fd < 0) {
    perror("socket");
    exit(1);
}

// configura o endereço do servidor
memset(&serv_addr, 0, sizeof(serv_addr));
port_num = atoi(argv[1]);
serv_addr.sin_family = AF_INET;
serv_addr.sin_addr.s_addr = INADDR_ANY;
serv_addr.sin_port = htons(port_num);

// associa o socket à porta do servidor
if (bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    perror("bind");
    exit(1);
}

// define o socket para ouvir conexões
listen(sock_fd, MAX_CLIENTS);

// loop principal do servidor
while (1) {
    clilen = sizeof(cli_addr);
    new_sock_fd = accept(sock_fd, (struct sockaddr *) &cli_addr, &clilen);
    if (new_sock_fd < 0) {
        perror("accept");
        continue;
    }

    // cria um novo cliente
    Cliente cliente;
    cliente.sock_fd = new_sock_fd;
    if (autenticar_cliente(new_sock_fd, cliente.nome) == 0) {
        close(new_sock_fd);
        continue;
    }

    // espera o cliente informar o tópico ao qual deseja se conectar
    char buf[MAX_BUF_SIZE];
    int n = read(new_sock_fd, buf, MAX_BUF_SIZE-1);
    if (n < 0) {
        perror("read");
        close(new_sock_fd);
        continue;
    }
    buf[n] = '\0';

    // procura o tópico informado pelo cliente
    Topico *topico = NULL;
    for (i = 0; i < MAX_TOPICOS; i++) {
        if (strcmp(topicos[i].nome, buf) == 0) {
            topico = &topicos[i];
            break;
        }
    }

    // se o tópico não existir, cria um novo tópico
    if (topico == NULL) {
        for (i = 0; i < MAX_TOPICOS; i++) {
            if (topicos[i].num_clientes == 0) {
                strncpy(topicos[i].nome, buf, 19);
                topicos[i].nome[19] = '\0';
                strncpy(topicos[i].endereco_multicast, "224.0.0.1", 19);
                topicos[i].endereco_multicast[19] = '\0';
                topico = &topicos[i];
                break;
            }
        }
    }