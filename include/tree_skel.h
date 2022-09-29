#ifndef _TREE_SKEL_H
#define _TREE_SKEL_H

#include "sdmessage.pb-c.h"
#include "tree.h"
#include "message-private.h"

struct task_t {
  int op_n; //o número da operação
  int op; //a operação a executar. op=0 se for um delete, op=1 se for um put
  char* key; //a chave a remover ou adicionar
  char* data; // os dados a adicionar em caso de put, ou NULL em caso de delete
  struct task_t *next_task;
};

/* Inicia o skeleton da árvore.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke().
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
 */
int tree_skel_init();

/* Liberta toda a memória e recursos alocados pela função tree_skel_init.
 */
void tree_skel_destroy();

/* Executa uma operação na árvore (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura message_t para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, árvore nao incializada)
*/
int invoke(struct message_t *msg);

/* Verifica se a operação identificada por op_n foi executada.
*/
int verify(int op_n);

int zookeeper_connect(char* zooAdr, char* myip);

int zookeeper_disconnect();

#endif
