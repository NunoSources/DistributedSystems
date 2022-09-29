// Grupo 14
// Alexandre Torrinha fc52757
// Nuno Fontes fc46413
// Rodrigo Simões fc52758

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "network_server.h"
#include "tree_skel.h"

void handler(int s) {
  printf("A terminar execucao...");
  network_server_close();
  exit(0);
}

void create_catcher() {
  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);

  struct sigaction sigPipeHandler;

	sigPipeHandler.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sigPipeHandler, NULL);
}

int main(int argc, char *argv[]) {
  // Ler port do argumento
  int address_port = atoi(argv[1]);
  char* zooAdr = argv[2]; // <IP>:<porta>

  // Iniciar socket
  int socket = network_server_init(address_port, zooAdr);
  if(socket == -1) {
    return -1;
  }

  create_catcher();

  printf("Ah escuta de clientes...\n");
  while(1) {
    network_main_loop(socket);
  }

  return 0;
}
