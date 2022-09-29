// Grupo 14
// Alexandre Torrinha fc52757
// Nuno Fontes fc46413
// Rodrigo Simões fc52758

#include <string.h>
#include <stdlib.h>
#include <inet.h>

#include "client_stub.h"
#include "network_client.h"
#include "client_stub-private.h"
#include "message-private.h"
#include "zookeeper/zookeeper.h"

// O stub implementa e disponibiliza uma função de adaptação para cada operação remota que o
// cliente possa efetuar no servidor, bem como funções para estabelecer e terminar a ligação a um
// servidor. Cada função de adaptação vai preencher uma estrutura message_t com o opcode
// (tipo da operação), com o tipo de conteúdo (c_type) necessário para realizar a operação, e
// com esse conteúdo. Esta estrutura é então passada para o módulo de comunicação, que a irá
// processar e devolver uma resposta (veja a próxima secção).

/* Receives argv and puts argv[1] into an array of strings
*/
char** get_server_info(const char *address_port) {
  if(address_port == NULL) {
    return NULL;
  }
  const char d[2] = ":";
  char** connection_info = malloc(sizeof(char*) * 2);
  char* token;

  // argv[1] on the form <server>:<port>

  // Gets the servers name or adress on the argv[1]
  token = strtok((char*) address_port, d);
  if(token == NULL) {
    return NULL;
  }
  connection_info[0] = strdup(token);

  // Gets the TCP port on the argv[1]
  token = strtok(NULL, d);
  if(token == NULL) {
    return NULL;
  }
  connection_info[1] = strdup(token);

  return connection_info;
}

struct rtree_t *rtree_connect(const char *address_port) {
  struct rtree_t *remote_tree = malloc(sizeof(struct rtree_t));

  char** connection_info = get_server_info(address_port);
  if(connection_info == NULL) {
    return NULL;
  }
  remote_tree->address = connection_info[0];
  remote_tree->port = connection_info[1];
  int result = network_connect(remote_tree);
  if(result == -1) {
    return NULL;
  }
  return remote_tree;
}

int rtree_disconnect(struct rtree_t *rtree) {
  free(rtree->address);
  free(rtree->port);
  int result = rtree_disconnect(rtree);
  if(result == -1) {
    return -1;
  }
  return 0;
}

int rtree_put(struct rtree_t *rtree, struct entry_t *entry) {
  MessageT msgC;
  message_t__init(&msgC);
  DataM dataC = DATA_M__INIT;
  EntryM entryC = ENTRY_M__INIT;
  struct message_t *msg = malloc(sizeof(struct message_t));
  msg->content = &msgC;

  msg->content->opcode = MESSAGE_T__OPCODE__OP_PUT;
  msg->content->c_type = MESSAGE_T__C_TYPE__CT_ENTRY;

  entryC.key = malloc(strlen(entry->key) + 1);
  strcpy(entryC.key, entry->key);
  msg->content->entry = &entryC;

  ProtobufCBinaryData bin;
  bin.data = malloc(entry->value->datasize);
  memcpy(bin.data, entry->value->data, entry->value->datasize);
  bin.len = entry->value->datasize;
  dataC.data = bin;
  msg->content->entry->data = &dataC;

  struct message_t *recv_msg = network_send_receive(rtree, msg);
  free(msg);

  if(recv_msg->content->opcode != MESSAGE_T__OPCODE__OP_PUT + 1 ||
    recv_msg->content->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
    return -1;
  }

  int received_id = ntohl(recv_msg->content->data_size);
  message_t__free_unpacked(recv_msg->content, NULL);
  return received_id;
}

struct data_t *rtree_get(struct rtree_t *rtree, char *key) {
  MessageT msgC;
  message_t__init(&msgC);
  DataM dataC = DATA_M__INIT;
  EntryM entryC = ENTRY_M__INIT;

  struct message_t *msg = malloc(sizeof(struct message_t));
  msg->content = &msgC;

  msg->content->entry = &entryC;
  msg->content->entry->data = &dataC;

  msg->content->opcode = MESSAGE_T__OPCODE__OP_GET;
  msg->content->c_type = MESSAGE_T__C_TYPE__CT_KEY;

  msg->content->data_size = sizeof(char) * strlen(key);
  msg->content->n_data = 1;
  msg->content->data = malloc(sizeof(char *) * 1);
  msg->content->data[0] = malloc(strlen(key) + 1);
  strcpy(msg->content->data[0], key);

  struct message_t *recv_msg = network_send_receive(rtree, msg);

  if(recv_msg->content->opcode != MESSAGE_T__OPCODE__OP_GET + 1 ||
    recv_msg->content->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
    return NULL;
  }
  void* new_data = malloc(recv_msg->content->entry->data->data.len);
  memcpy(new_data, recv_msg->content->entry->data->data.data, recv_msg->content->entry->data->data.len);
  struct data_t *received_data = data_create2(recv_msg->content->entry->data->data.len,
    new_data);
  message_t__free_unpacked(recv_msg->content, NULL);

  return received_data;
}

int rtree_del(struct rtree_t *rtree, char *key) {
  MessageT msgC;
  message_t__init(&msgC);
  struct message_t *msg = malloc(sizeof(struct message_t));
  msg->content = &msgC;

  msg->content->opcode = MESSAGE_T__OPCODE__OP_DEL;
  msg->content->c_type = MESSAGE_T__C_TYPE__CT_KEY;

  msg->content->data_size = sizeof(char) * strlen(key) + 1;
  msg->content->n_data = 1;
  msg->content->data = malloc(sizeof(char *) * 1);
  msg->content->data[0] = malloc(strlen(key) + 1);
  strcpy(msg->content->data[0], key);

  struct message_t *recv_msg = network_send_receive(rtree, msg);
  if(recv_msg->content->opcode != MESSAGE_T__OPCODE__OP_DEL + 1 ||
    recv_msg->content->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
    return -1;
  }
  int received_id = ntohl(recv_msg->content->data_size);
  message_t__free_unpacked(recv_msg->content, NULL);

  return received_id;
}

int rtree_size(struct rtree_t *rtree) {
  MessageT msgC;
  message_t__init(&msgC);
  struct message_t *msg = malloc(sizeof(struct message_t));
  msg->content = &msgC;

  msg->content->opcode = MESSAGE_T__OPCODE__OP_SIZE;
  msg->content->c_type = MESSAGE_T__C_TYPE__CT_NONE;

  struct message_t *recv_msg = network_send_receive(rtree, msg);
  free(msg);

  if(recv_msg->content->opcode != MESSAGE_T__OPCODE__OP_SIZE + 1 ||
    recv_msg->content->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
    return -1;
  }
  int received_size = ntohl(recv_msg->content->data_size);
  message_t__free_unpacked(recv_msg->content, NULL);

  return received_size;
}

int rtree_height(struct rtree_t *rtree) {
  MessageT msgC;
  message_t__init(&msgC);
  struct message_t *msg = malloc(sizeof(struct message_t));
  msg->content = &msgC;

  msg->content->opcode = MESSAGE_T__OPCODE__OP_HEIGHT;
  msg->content->c_type = MESSAGE_T__C_TYPE__CT_NONE;
  msg->content->data_size = 0;
  msg->content->data = NULL;

  struct message_t *recv_msg = network_send_receive(rtree, msg);
  free(msg);
  if(recv_msg->content->opcode != MESSAGE_T__OPCODE__OP_HEIGHT + 1 ||
    recv_msg->content->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
    return -1;
  }

  int received_size = ntohl(recv_msg->content->data_size);
  message_t__free_unpacked(recv_msg->content, NULL);
  return received_size;
}

char **rtree_get_keys(struct rtree_t *rtree) {
  MessageT msgC;
  message_t__init(&msgC);
  struct message_t *msg = malloc(sizeof(struct message_t));
  msg->content = &msgC;

  msg->content->opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
  msg->content->c_type = MESSAGE_T__C_TYPE__CT_NONE;
  msg->content->data_size = 0;
  msg->content->data = NULL;

  struct message_t *recv_msg = network_send_receive(rtree, msg);
  free(msg);
  if(recv_msg->content->opcode != MESSAGE_T__OPCODE__OP_GETKEYS + 1 ||
    recv_msg->content->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
    return NULL;
  }

  char** keys = malloc((recv_msg->content->n_data + 1) * sizeof(char*));
  for(int i = 0; i < recv_msg->content->n_data; i++) {
    keys[i] = malloc(strlen(recv_msg->content->data[i]) + 1);
    strcpy(keys[i], recv_msg->content->data[i]);
  }
  keys[recv_msg->content->n_data] = NULL;

  message_t__free_unpacked(recv_msg->content, NULL);
  return keys;
}

int get_array_size2(char **keys) {
    int i = 0;
    while(keys[i] != NULL) {
        i++;
    }
    return i;
}

void rtree_free_keys(char **keys) {
  int n = get_array_size2(keys);
  for (int i=0; i < n; i++) {
    free(keys[i]);
  }
  free(keys);
  return;
}

int rtree_verify(struct rtree_t *rtree, int op_n) {
  MessageT msgC;
  message_t__init(&msgC);
  struct message_t *msg = malloc(sizeof(struct message_t *));
  msg->content = &msgC;

  msg->content->opcode = MESSAGE_T__OPCODE__OP_VERIFY;
  msg->content->c_type = MESSAGE_T__C_TYPE__CT_RESULT;

  msg->content->data_size = op_n;

  struct message_t *recv_msg = network_send_receive(rtree, msg);
  if(recv_msg->content->opcode != MESSAGE_T__OPCODE__OP_VERIFY + 1 ||
    recv_msg->content->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
    return -1;
  }
  int received_size = ntohl(recv_msg->content->data_size);
  message_t__free_unpacked(recv_msg->content, NULL);

  return received_size;
}
