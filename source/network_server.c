// Grupo 14
// Alexandre Torrinha fc52757
// Nuno Fontes fc46413
// Rodrigo Simões fc52758

#include "sdmessage.pb-c.h"
#include "inet.h"
#include "network_server.h"
#include "message-private.h"

#include <stdlib.h>
#include <fcntl.h>
#include <poll.h>

#define NFDESC 500 // N�mero de sockets (uma para listening)
#define TIMEOUT 50 // em milisegundos

// Socket global
int sockfd;

/* Função para preparar uma socket de receção de pedidos de ligação
 * num determinado porto.
 * Retornar descritor do socket (OK) ou -1 (erro).
 */
int network_server_init(short port, char* zooAdr){
  struct sockaddr_in server;

  if(port == 0){
    return -1;
  }

  // Cria socket TCP
  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    perror("Erro ao criar socket");
    return -1;
  }

  // Preenche estrutura server para bind
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  // Faz bind
  if(bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0){
    perror("Erro ao fazer bind");
    close(sockfd);
    return -1;
  }

  // Faz listen
  if(listen(sockfd, 0) < 0){
    perror("Erro ao executar listen");
    close(sockfd);
    return -1;
  }

  //Inicializar tree
  int result = tree_skel_init(zooAdr);
  if(result == -1) {
    close(sockfd);
    return -1;
  }

  //Inicializar ZooKeeper
  char* myip = malloc(sizeof(char*) * 15);
  sprintf(myip, "127.0.0.1:%d", port);
  result = zookeeper_connect(zooAdr, myip);
  free(myip);


  return sockfd;
}

int network_main_loop(int listening_socket){
  struct pollfd connections[NFDESC]; // Estrutura para file descriptors das sockets das ligacoes
  struct sockaddr_in client; // estruturas para os dados dos endere�os de servidor e cliente
  struct in_addr ipAddr;
  char client_ip[INET_ADDRSTRLEN];
  char client_addr[INET_ADDRSTRLEN+5];

  int nfds, kfds, i;
  socklen_t size_client;

  for (i = 0; i < NFDESC; i++)
    connections[i].fd = -1;    // poll ignora estruturas com fd < 0

  connections[0].fd = sockfd;  // Vamos detetar eventos na welcoming socket
  connections[0].events = POLLIN;  // Vamos esperar ligacoes nesta socket

  nfds = 1; // numero de file descriptors

  // Retorna assim que exista um evento ou que TIMEOUT expire. * FUNCAO POLL *.
  while ((kfds = poll(connections, nfds, 10)) >= 0) {// kfds == 0 significa timeout sem eventos

    if (kfds > 0){ // kfds e o numero de descritores com evento ou erro

      if ((connections[0].revents & POLLIN) && (nfds < NFDESC)) { // Pedido na listening socket ?
        if ((connections[nfds].fd = accept(connections[0].fd, (struct sockaddr *) &client, &size_client)) > 0){ // Ligacaoo feita
          ipAddr = client.sin_addr;
          inet_ntop( AF_INET, &ipAddr, client_ip, INET_ADDRSTRLEN );
          sprintf(client_addr, "%s:%d", client_ip, (int) ntohs(client.sin_port));
          connections[nfds].events = POLLIN; // Vamos esperar dados nesta socket
          printf("Nova ligacao numero %d de %s\n", nfds, client_addr);
          nfds++;
        }
      }

      for (i = 1; i < nfds; i++) {// Todas as ligacoes

        if (connections[i].revents & POLLIN) { // Dados para ler ?
          // Le string enviada pelo cliente do socket referente ah conexao i
          struct message_t *received_msg = network_receive(connections[i].fd);
          if(received_msg == NULL) {
            perror("Erro ao receber dados do cliente");
            close(connections[i].fd);
            connections[i].fd = -1;
            continue;
          }

          invoke(received_msg);
          struct message_t *response_msg = received_msg;
          int result = network_send(connections[i].fd, (struct message_t*) response_msg);
          if(result == -1) {
            perror("Erro ao enviar resposta ao cliente");
            close(connections[i].fd);
            connections[i].fd = -1;
            continue;
          }
        }
      }
    }
  }

  // Fecha socket de listening (so executado em caso de erro...)
  close(sockfd);
  return 0;
}

/* Esta função deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura message_t.
 */
struct message_t *network_receive(int client_socket){
  int msg_size = 0;
  read_all(client_socket, (uint8_t *) &msg_size, sizeof(int));

  char *buffer = malloc(msg_size);

  int read_size = read_all(client_socket, (uint8_t *) buffer, msg_size);

  if(read_size <= 0) return NULL;

  MessageT *request = NULL;
  request = message_t__unpack(NULL, read_size, (uint8_t *) buffer);
  free(buffer);
  if(request == NULL) {
    fprintf(stdout, "error unpacking message\n");
    return NULL;
  }

  struct message_t *request_msg = malloc(sizeof(struct message_t *));
  request_msg->content = request;

  return request_msg;
}

/* Esta função deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Libertar a memória ocupada por esta mensagem;
 * - Enviar a mensagem serializada, através do client_socket.
 */
int network_send(int client_socket, struct message_t *msg){
  int len = message_t__get_packed_size(msg->content);

  write_all(client_socket, (uint8_t *) &len, sizeof(int));

  uint8_t *bufint = malloc(len);

  message_t__pack(msg->content, bufint);
  message_t__free_unpacked(msg->content, NULL);
  free(msg);

  int buff_size = write_all(client_socket, bufint, len);
  free(bufint);
  if(buff_size <= 0) return -1;

  return 0;
}

/* A função network_server_close() liberta os recursos alocados por
 * network_server_init().
 */
int network_server_close(){
  tree_skel_destroy();
  zookeeper_disconnect();
  close(sockfd);
  return 0;
}
