// Grupo 14
// Alexandre Torrinha fc52757
// Nuno Fontes fc46413
// Rodrigo Simões fc52758

#include "network_client.h"
#include "inet.h"
#include "message-private.h"
#include "client_stub-private.h"

#include <fcntl.h>

/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) a base da
 *   informação guardada na estrutura rtree;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rtree;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rtree_t *rtree){
  struct sockaddr_in server; /*Obter o endereço do servidor (struct sockaddr_in) a base da
                                  informação guardada na estrutura rtree;*/
  if(rtree == NULL){
    return -1;
  }

  //Cria socket TCP
  if((rtree->socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    perror("Erro ao criar socket");
    return -1;
  }

  //Preenche estrutura server para estabelecer conexão
  server.sin_family = AF_INET;
  server.sin_port = htons(atoi(rtree->port));
  if(inet_pton(AF_INET, rtree->address, &server.sin_addr) < 1){
    printf("Erro ao converter IP\n");
    close(rtree->socketfd);
    return-1;
  }
  //Conecta com o servidor
  if(connect(rtree->socketfd,(struct sockaddr *)&server, sizeof(server)) < 0){
    perror("Erro ao conectar-se ao servidor");
    close(rtree->socketfd);
    return -1;
  }

  return 0;
}

/* Esta função deve:
 * - Obter o descritor da ligação (socket) da estrutura rtree_t;
 * - Serializar a mensagem contida em msg;
 * - Enviar a mensagem serializada para o servidor;
 * - Esperar a resposta do servidor;
 * - De-serializar a mensagem de resposta;
 * - Retornar a mensagem de-serializada ou NULL em caso de erro.
 */
struct message_t *network_send_receive(struct rtree_t * rtree, struct message_t *msg){
  if(rtree == NULL || msg == NULL){
    return NULL;
  }

  //  Send message
  int len = message_t__get_packed_size(msg->content);

  write_all(rtree->socketfd, (uint8_t *) &len, sizeof(int));

  uint8_t * buf = malloc(len);
  if(buf == NULL) {
    fprintf(stdout, "malloc error\n");
    return NULL;
  }

  message_t__pack(msg->content, (uint8_t *) buf);
  int buff_size = write_all(rtree->socketfd, buf, len);
  if(buff_size <= 0) return NULL;
  free(buf);

  // Receive response
  len = MAX_MSG;
  buf = malloc(len);

  int msg_size = 0;
  read_all(rtree->socketfd, (uint8_t *) &msg_size, sizeof(int));

  int read_size = read_all(rtree->socketfd, buf, msg_size);
  if(read_size <= 0) return NULL;

  MessageT *msg_received;

  msg_received = message_t__unpack(NULL, read_size, buf);
  if(msg_received == NULL) {
    fprintf(stdout, "error unpacking message\n");
    return NULL;
  }

  struct message_t *response = malloc(sizeof(struct message_t*));
  response->content = msg_received;
  return response;
}

/* A função network_close() fecha a ligação estabelecida por
 * network_connect().
 */
int network_close(struct rtree_t * rtree){
  if(rtree == NULL){
    return -1;
  }
  close(rtree->socketfd);
  return 0;
}
