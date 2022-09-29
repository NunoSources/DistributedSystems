// Grupo 14
// Alexandre Torrinha fc52757
// Nuno Fontes fc46413
// Rodrigo Simões fc52758

// Recebe comando stdin, invoca chamada remota pelo stub, imprime resposta e repete
// Usar fgets e strtok para ler e tratar comandos
// Comandos: put <key> <data> / get <key> / del <key> / size / height / getkeys / quit

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <entry.h>
#include <stdlib.h>
#include <inet.h>
#include "zookeeper/zookeeper.h"
#include "client_stub.h"

typedef struct String_vector zoo_string;

struct rtree_t *primary;
struct rtree_t *backup;
int send_commands = 1;

/*
 * A ação associada à receção de SIGPIPE passa a ser ignorar.
 */
int ignsigpipe(){
	struct sigaction s;

	s.sa_handler = SIG_IGN;
	return sigaction(SIGPIPE, &s, NULL);
}

char** get_command(char* command_str) {
  const char d[2] = " ";
  char* token;
  char** command = malloc(sizeof(char*) * 3);

  // Gets command name
  token = strtok(command_str, d);
  command[0] = strdup(token);

  // Gets command first argument
  token = strtok(NULL, d);
  if(token != NULL) {
    command[1] = strdup(token);
  } else {
    command[1] = NULL;
  }

  // Gets command last argument (no delimiter)
  token = strtok(NULL, "");
  if(token != NULL) {
    command[2] = strdup(token);
  } else {
    command[2] = NULL;
  }

  return command;
}

int do_command(char** command) {
  if(strcmp(command[0], "put") == 0) {
    struct data_t *new_data = data_create2(strlen(command[2]) + 1, command[2]);
    struct entry_t *new_entry = entry_create(command[1], new_data);
    int response = rtree_put(primary, new_entry);
    printf("%d\n", response);

  } else if(strcmp(command[0], "get") == 0) {
    struct data_t *target = rtree_get(backup, command[1]);
    if(target == NULL) {
      printf("Não encontrado\n");
    } else {
      printf("%s\n", (char*) target->data);
      data_destroy(target);
    }

  } else if(strcmp(command[0], "del") == 0) {
    int response = rtree_del(primary, command[1]);
    printf("%d\n", response);

  } else if(strcmp(command[0], "size") == 0) {
    int response = rtree_size(backup);
    printf("%d\n", response);

  } else if(strcmp(command[0], "height") == 0) {
    int response = rtree_height(backup);
    printf("%d\n", response);

  } else if(strcmp(command[0], "getkeys") == 0) {
    char **keys = rtree_get_keys(backup);
    int i = 0;
    printf("[");
    while(keys[i] != NULL) {
      if(i != 0) {
        printf(",");
      }
      printf("%s", keys[i]);
      i++;
    }
    printf("]\n");
    rtree_free_keys(keys);

  } else if(strcmp(command[0], "verify") == 0) {
    int response = rtree_verify(backup, atoi(command[1]));
    printf(response ? "true\n" : "false\n");

  } else if(strcmp(command[0], "quit") == 0) {
    return 1;

  } else {
    printf("Invalid command!\n");
  }
  return 0;
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

static void child_watcher(zhandle_t *zh, int type, int state, const char *zpath, void *watcher_ctx) {
  // verificar se houve uma saída ou entrada de algum servidor
	zoo_string* children_list =	(zoo_string *) malloc(sizeof(zoo_string));
	// int zoo_data_len = ZDATALEN;
	if (state == ZOO_CONNECTED_STATE)	 {
		if (type == ZOO_CHILD_EVENT) {
	 	   /* Get the updated children and reset the watch */
 			if (ZOK != zoo_wget_children(zh, "/kvstore", &child_watcher, watcher_ctx, children_list)) {
 				fprintf(stderr, "Error setting watch at %s!\n", "/kvstore");
 			}
		 }
	 }

	 // Verificar se o primary existe
   int notfoundP = !findString(children_list, "primary");
   // Verificar se o backup existe
   int notfoundB = !findString(children_list, "backup");
	 free(children_list);

	 int zdata_len = 22;
	 char *prim_addr = malloc(zdata_len);
	 char *back_addr = malloc(zdata_len);

	 //Se saiu um dos servidores, atualizar a estrutura de dados correspondente
	 if(notfoundP) {
		 free(primary);
		 send_commands = 0;
	 } else if(!notfoundP) {
	 		// Obter os meta-dados do primary
			zoo_get(zh, "/kvstore/primary", 0, prim_addr, &zdata_len, NULL);
			sleep(1);
			primary = rtree_connect(prim_addr);
			if(primary == NULL) {
				perror("Erro ao estabelecer ligacao ao servidor primary");
				exit(-1);
 			}
	 }

	 if(notfoundB) {
		 free(backup);
		 send_commands = 0;
	 } else if(!notfoundB) {
		 zoo_get(zh, "/kvstore/backup", 0, back_addr, &zdata_len, NULL);
		 sleep(1);
		 backup = rtree_connect(back_addr);
		 if(backup == NULL) {
			 perror("Erro ao estabelecer ligacao ao servidor backup");
			 exit(-1);
		 }
		 send_commands = 1;
	 }

	 free(prim_addr);
	 free(back_addr);

	 // Se ambos os servidores estiverem ativos, ativar envio de comandos
	 if(!notfoundP && !notfoundB) {
		 send_commands = 1;
	 }

}

int main(int argc, char *argv[]) {
	// Ligar ao ZooKeeper
  const char* address_port = argv[1];
	zoo_set_debug_level(1);
	zhandle_t *zh = zookeeper_init(address_port, my_watcher_func,	2000, 0, NULL, 0);

	//Obter e fazer watch aos filhos de /kvstore;
	zoo_string* children_list =	(zoo_string *) malloc(sizeof(zoo_string));

  if (ZOK != zoo_wget_children(zh, "/kvstore", &child_watcher, NULL, children_list)) {
    fprintf(stderr, "Error setting watch at %s!\n", "/kvstore");
    return -1;
  }


	// Dos filhos de /kvstore, obter do ZooKeeper os pares IP:porta dos servidores primário e
	// secundário, guardá-los como primary e backup respectivamente
	int zdata_len = 22;
	char *prim_addr = malloc(zdata_len);
	char *back_addr = malloc(zdata_len);

	// Obter os meta-dados do primary
	int foundP = zoo_get(zh, "/kvstore/primary", 0, prim_addr, &zdata_len, NULL);
	if (ZNONODE == foundP) {
		send_commands = 0;
	} else if(ZOK == foundP) {
		// e ligar a eles;
	  primary = rtree_connect(prim_addr);
		if(primary == NULL) {
			perror("Erro ao estabelecer ligacao ao servidor primary");
			free(prim_addr);
			exit(-1);
		}
	} else {
		fprintf(stderr, "Error setting watch at %s!\n", "/kvstore/primary");
		free(prim_addr);
		exit(-1);
	}
	free(prim_addr);

	// Obter os meta-dados do backup
	int foundB = zoo_get(zh, "/kvstore/backup", 0, back_addr, &zdata_len, NULL);
	if (ZNONODE == foundB) {
		send_commands = 0;
	} else if(ZOK == foundB) {
		// e ligar a eles;
		sleep(1);
		backup = rtree_connect(back_addr);
		if(backup == NULL) {
			perror("Erro ao estabelecer ligacao ao servidor backup");
			free(back_addr);
			exit(-1);
		}
	} else {
		fprintf(stderr, "Error setting watch at %s!\n", "/kvstore/backup");
		free(back_addr);
		exit(-1);
	}
	free(back_addr);

  if (ignsigpipe() != 0){ // Alterar comportamento face a SIGPIPE
			perror("ignsigpipe falhou");
			exit(1);
	}

  char *command_str = malloc(sizeof(char) * MAX_MSG);

  int quit = 0;
  while(!quit) {
    printf( "Enter a command: ");
    fgets(command_str, MAX_MSG, stdin);
    command_str = strtok(command_str, "\n");
    char** command = get_command(command_str);
		if(send_commands) {
    	quit = do_command(command);
		} else {
			printf("Primary or backup servers are not available.\n");
		}
    free(command);
  }
  free(command_str);

	return 0;
}
