// Grupo 14
// Alexandre Torrinha fc52757
// Nuno Fontes fc46413
// Rodrigo Simões fc52758

#include "tree_skel.h"
#include "data.h"
#include "entry.h"
#include "zookeeper/zookeeper.h"
#include "client_stub-private.h"
#include "client_stub.h"

#include <pthread.h>
#include <signal.h>

#define ZDATALEN 1024 * 1024
typedef struct String_vector zoo_string;

struct tree_t *tree;
struct rtree_t *rtree;
zhandle_t *zh;
char* my_id;
char* other_id;
int accept_requests = 0;

int last_assigned = 0;
int op_count = 0;

struct task_t *c_task;
pthread_mutex_t queue_lock, tree_lock;
pthread_cond_t queue_not_empty;

pthread_t tid;
int exit_thread;

int send_task(struct task_t *task) {
  int result;
  if(task->op == 1) {
    struct data_t *data = data_create2(strlen(task->data), task->data);
    struct entry_t *entry = entry_create(task->key, data);
    result = rtree_put(rtree, entry);
  } else {
    result = rtree_del(rtree, task->key);
  }
  return result;
}

void task_done() {
  sleep(1);
  if(strcmp(my_id, "primary") == 0) {
    send_task(c_task);
  }
  if(c_task->next_task == NULL) {
    c_task = NULL;
  } else {
    struct task_t *next_task = c_task->next_task;
    free(c_task->key);
    free(c_task);
    c_task = next_task;
  }
  op_count++;
}

int add_task(struct task_t *new_task) {
  if(strcmp(my_id, "primary") == 0 && !accept_requests) {
    return -1;
  }
  pthread_mutex_lock(&queue_lock);
  if(c_task == NULL) {
    c_task = new_task;
    new_task->next_task = NULL;
  } else {
    struct task_t *selected_task = c_task;
    while(selected_task->next_task != NULL) {
      selected_task = selected_task->next_task;
    }
    selected_task->next_task = new_task;
    new_task->next_task = NULL;
  }
  last_assigned++;
  pthread_cond_signal(&queue_not_empty);
  pthread_mutex_unlock(&queue_lock);
  return last_assigned - 1;
}

void mask_sig(void) {
	sigset_t mask;
	sigemptyset(&mask);
  sigaddset(&mask, SIGRTMIN+3);

  pthread_sigmask(SIG_BLOCK, &mask, NULL);
}

void *process_task(void *params) {
  mask_sig();
  int result = 0;
  while(!exit_thread) {
    pthread_mutex_lock(&queue_lock);
    pthread_cond_wait(&queue_not_empty, &queue_lock);
    if(exit_thread) {
      pthread_exit(NULL);
    }
    if(c_task->op == 1) {
      struct data_t *c_data = data_create2(strlen(c_task->data), c_task->data);
      printf("A adicionar %s na arvore\n", c_task->key);
      result = tree_put(tree, c_task->key, c_data);
      free(c_data);
    } else {
      printf("A remover %s da arvore\n", c_task->key);
      result = tree_del(tree, c_task->key);
    }
    if(result != -1) {
      task_done();
    } else {
      printf("Erro na operacao\n");
      task_done();
    }
    pthread_mutex_unlock(&queue_lock);
  }
  pthread_exit(NULL);
}

int tree_skel_init() {
  tree = tree_create();
  rtree = malloc(sizeof(struct rtree_t));
  if(tree == NULL) {
    return -1;
  }

  if(tree == NULL) {
    perror("Criacao de arvore falhou");
    return -1;
  }
  if(pthread_mutex_init(&queue_lock, NULL) == -1) {
    return -1;
  }
  if(pthread_mutex_init(&tree_lock, NULL) == -1) {
    return -1;
  }
  if(pthread_cond_init(&queue_not_empty, NULL) == -1) {
    return -1;
  }

  exit_thread = 0;

  if(pthread_create(&tid, NULL, process_task, NULL) != 0) {
    perror("pthread_create");
		exit(1);
  }
  if(pthread_detach(tid) != 0) {
    perror("pthread_detach");
		exit(1);
  }


  return 0;
}

void tree_skel_destroy() {
  exit_thread = 1;
  pthread_kill(tid,SIGINT);
  pthread_join(tid, NULL);

  tree_destroy(tree);

  pthread_mutex_destroy(&queue_lock);
  pthread_mutex_destroy(&tree_lock);
  pthread_cond_destroy(&queue_not_empty);
}

int invoke_put(MessageT *msg) {
  struct task_t* new_task = (struct task_t*) malloc(sizeof(struct task_t));
  new_task->op_n = last_assigned;
  new_task->op = 1;
  new_task->key = malloc(strlen(msg->entry->key) + 1);
  strcpy(new_task->key, msg->entry->key);
  new_task->data = malloc(msg->entry->data->data.len);
  memcpy(new_task->data, (char*) msg->entry->data->data.data, msg->entry->data->data.len);



  int op_id = add_task(new_task);

  msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
  if(op_id == -1) {
    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
    return -1;
  } else {
    msg->opcode += 1;
  }

  msg->data_size = htonl(op_id);

  return op_id;
}

int invoke_get(MessageT *msg) {
  struct data_t *result = tree_get(tree, msg->data[0]);
  if(result == NULL) {
    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
    return -1;
  }

  msg->entry->data->data.data = result->data;
  msg->entry->data->data.len = result->datasize;

  msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;
  msg->opcode += 1;

  return 0;
}

int invoke_del(MessageT *msg) {
  struct task_t* new_task = malloc(sizeof(struct task_t));
  new_task->op_n = last_assigned;
  new_task->op = 0;
  new_task->key = malloc(strlen(msg->data[0]) + 1);
  strcpy(new_task->key, msg->data[0]);

  int op_id = add_task(new_task);


  msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
  if(op_id == -1) {
    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
    return -1;
  } else {
    msg->opcode += 1;
  }

  msg->data_size = htonl(op_id);

  return op_id;
}

int invoke_height(MessageT *msg) {
  int result = tree_height(tree);
  if(result == -1) {
    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
    return -1;
  }
  msg->data_size = htonl(result);
  msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
  msg->opcode += 1;

  return 0;
}

int invoke_size(MessageT *msg) {
  int result = tree_size(tree);
  if(result == -1) {
    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
    return -1;
  }
  msg->data_size = htonl(result);
  msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
  msg->opcode += 1;

  return 0;
}

int invoke_getkeys(MessageT *msg) {
  char** result = tree_get_keys(tree);
  if(result == NULL) {
    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
    return -1;
  }
  int i = 0;
  int counter = 0;
  while(result[i] != NULL) {
    counter++;
    i++;
  }
  msg->n_data = counter;
  msg->data = result;
  msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;
  msg->opcode += 1;

  return 0;
}

int verify(int op_n) {
  if(op_n > last_assigned) {
    return -1;
  }
  return op_n < last_assigned;
}

int invoke_verify(MessageT *msg) {
  int result = verify(msg->data_size);
  if(result == -1) {
    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
    return -1;
  }

  msg->data_size = htonl(result);
  msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
  msg->opcode += 1;

  return 0;
}

int invoke_senderror(MessageT *msg) {
  msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
  return -1;
}

int invoke(struct message_t *msg) {
  MessageT *content = msg->content;
  int result = 0;

  if(strcmp(my_id, "primary") == 0) {
    if(accept_requests) {
      switch (content->opcode) {
        case MESSAGE_T__OPCODE__OP_PUT:
          result = invoke_put(content);
          break;
        case MESSAGE_T__OPCODE__OP_DEL:
          result = invoke_del(content);
          break;
        default:
          result = invoke_senderror(content);
        }
      } else {
        result = invoke_senderror(content);
      }
  } else {
    switch (content->opcode) {
      case MESSAGE_T__OPCODE__OP_PUT:
        result = invoke_put(content);
        break;
      case MESSAGE_T__OPCODE__OP_GET:
        result = invoke_get(content);
        break;
      case MESSAGE_T__OPCODE__OP_DEL:
        result = invoke_del(content);
        break;
      case MESSAGE_T__OPCODE__OP_HEIGHT:
        result = invoke_height(content);
        break;
      case MESSAGE_T__OPCODE__OP_SIZE:
        result = invoke_size(content);
        break;
      case MESSAGE_T__OPCODE__OP_GETKEYS:
        result = invoke_getkeys(content);
        break;
      case MESSAGE_T__OPCODE__OP_VERIFY:
        result = invoke_verify(content);
        break;
      default:
        result = invoke_senderror(content);
    }
  }

  return result;
}

void my_watcher_func(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx) {}

int findString(zoo_string* list, char* string) {
  for(int i = 0; i < list->count; i++) {
    if(strcmp(list->data[i], string) == 0) {
      return 1;
    }
  }
  return 0;
}

/*
* Receives argv and puts argv[1] into an array of strings
*/
char** get_server_info2(const char *address_port) {
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

// Quando watch de filhos de /kvstore é ativada
static void child_watcher(zhandle_t *zhc, int type, int state, const char *zpath, void *watcher_ctx) {
  // verificar se houve uma saída ou entrada de algum servidor
	zoo_string* children_list =	(zoo_string *) malloc(sizeof(zoo_string));
	// int zoo_data_len = ZDATALEN;
	if (state == ZOO_CONNECTED_STATE)	 {
		if (type == ZOO_CHILD_EVENT) {
	 	   /* Get the updated children and reset the watch */
 			if (ZOK != zoo_wget_children(zhc, "/kvstore", &child_watcher, watcher_ctx, children_list)) {
 				fprintf(stderr, "Error setting watch at %s!\n", "/kvstore");
 			}
		 }
	 }

   // Verificar se o primary existe
   int notfoundP = !findString(children_list, "primary");
   // Verificar se o backup existe
   int notfoundB = !findString(children_list, "backup");

   // Se for servidor primário e o backup tiver saído
   if((strcmp(my_id, "primary") == 0) && notfoundB) {
     // não aceita mais pedidos de escrita dos clientes até que volte a haver backup
     accept_requests = 0;

   // Se for servidor backup e o primário tiver saído
   } else if((strcmp(my_id, "backup") == 0) && notfoundP) {
     // autopromove-se a servidor primário
     my_id = "primary";
     other_id = "backup";
     accept_requests = 0;

     int zdata_len = 22;
     char *myip = malloc(zdata_len);
     // Obter os meta-dados deste servidor
     if (ZOK != zoo_get(zh, "/kvstore/backup", 0, myip, &zdata_len, NULL)) {
       fprintf(stderr, "Error setting watch at %s!\n", "/kvstore/backup");
     }

     // Criar o node primary com o ip deste servidor
     if (ZOK != zoo_create(zh, "/kvstore/primary", myip, strlen(myip), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0)) {
       fprintf(stderr, "Error creating znode from path %s!\n", "/kvstore/primary");
       exit(EXIT_FAILURE);
     }

     // Apagar o node backup
     if (ZOK != zoo_delete(zh, "/kvstore/backup", -1)) {
       fprintf(stderr, "Error deleting znode from path %s!\n", "/kvstore/backup");
       exit(EXIT_FAILURE);
     }

     printf("Now I am primary!");

     // Se for servidor primário e houve ativação de backup Volta a ativar
     // watch.
   } else if((strcmp(my_id, "primary") == 0) && !notfoundB) {
     // guarda o seu par IP:porta,
     int zdata_len = 22;
     char *zdata_buf = malloc(zdata_len);
     // Obter os meta-dados do primary
     if (ZOK != zoo_get(zh, "/kvstore/backup", 0,zdata_buf, &zdata_len, NULL)) {
       fprintf(stderr, "Error setting watch at %s!\n", "/kvstore/primary");
     }
     // estabelece ligação
     // Espera que o servidor backup esteja pronto para aceitar ligacao
     // (tempo entre iniciar ZooKeeper e aceitar ligacoes)
     sleep(1);
     rtree = rtree_connect(zdata_buf);
     // e volta a aceitar pedidos de escrita dos clientes.
     accept_requests = 1;
   }


	 free(children_list);
}


int zookeeper_connect(char* zooAdr, char* myip) {
  // Ligar ao ZooKeeper
  zoo_set_debug_level(1);
  zh = zookeeper_init(zooAdr, my_watcher_func,	2000, 0, NULL, 0);
  if (zh == NULL)	{
		fprintf(stderr, "Error connecting to ZooKeeper server!\n");
    exit(EXIT_FAILURE);
	}

  // Verificar se o primary existe
  int notfoundP = (zoo_exists(zh, "/kvstore/primary", 0, NULL) == ZNONODE);
  // Verificar se o backup existe
  int notfoundB = (zoo_exists(zh, "/kvstore/backup", 0, NULL) == ZNONODE);

  // Se não existir o Znode /kvstore
  if (ZNONODE == zoo_exists(zh, "/kvstore", 0, NULL)) {
    // Criar esse Znode normal
    if (ZOK != zoo_create(zh, "/kvstore", NULL, 0, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0)) {
      fprintf(stderr, "Error creating znode from path %s!\n", "/kvstore");
      exit(EXIT_FAILURE);
    }
  }

  // Se o Znode /kvstore existir e tiver nós filhos /primary e /backup, terminar;
  if (!notfoundP && !notfoundB) {
    fprintf(stderr, "Primary and backup server already exist!\n");
    exit(EXIT_FAILURE);
  }

  // Se não existirem nós filhos de /kvstore
  if(notfoundP && notfoundB) {
    // criar o nó efémero /kvstore/primary,
    if (ZOK != zoo_create(zh, "/kvstore/primary", myip, strlen(myip), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0)) {
      fprintf(stderr, "Error creating znode from path %s!\n", "/kvstore/primary");
      exit(EXIT_FAILURE);
    }
    // assumindo-se este servidor como primário
    my_id = "primary";
    other_id = "backup";
    printf("I am primary!\n");
  }

  // Se existir o nó filho /primary e não existir o nó filho /backup
  if(!notfoundP && notfoundB) {
    // criar o nó efémero /kvstore/backup,
    if (ZOK != zoo_create(zh, "/kvstore/backup", myip, strlen(myip), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0)) {
      fprintf(stderr, "Error creating znode from path %s!\n", "/kvstore/backup");
      exit(EXIT_FAILURE);
    }
    // assumindo-se este servidor como backup
    my_id = "backup";
    other_id = "primary";
    printf("I am backup!\n");
  }

  // Se formos primary
  if((strcmp(my_id, "primary") == 0) && !notfoundB) {
    int zdata_len = 22;
  	char *zdata_buf = malloc(zdata_len);
    //Obter os meta-dados do backup
    if (ZOK != zoo_get(zh, "/kvstore/backup", 0,zdata_buf, &zdata_len, NULL)) {
      fprintf(stderr, "Error setting watch at %s!\n", "/kvstore/backup");
      return -1;
    }
    free(zdata_buf);
  // Se formos backup
  } else if(strcmp(my_id, "backup") == 0) {
    int zdata_len = 22;
  	char *zdata_buf = malloc(zdata_len);
    //Obter os meta-dados do primary
    if (ZOK != zoo_get(zh, "/kvstore/primary", 0,zdata_buf, &zdata_len, NULL)) {
      fprintf(stderr, "Error setting watch at %s!\n", "/kvstore/primary");
      return -1;
    }
    free(zdata_buf);
  // ou deixar a variável a NULL se o servidor for primário e
  // ainda não existir backup.
  } else {
    rtree  = NULL;
  }

  // Fazer watch aos filhos de /kvstore;
  if (ZOK != zoo_wget_children(zh, "/kvstore", &child_watcher, NULL, NULL)) {
    fprintf(stderr, "Error setting watch at %s!\n", "/kvstore");
    return -1;
  }

  return 0;
}

int zookeeper_disconnect() {
  zookeeper_close(zh);
  free(rtree);
  return 0;
}
