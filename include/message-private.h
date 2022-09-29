#ifndef _MESSAGE_H
#define _MESSAGE_H /* Módulo message */

#include <errno.h>
#include "sdmessage.pb-c.h"
#include "inet.h"

struct message_t
{
  MessageT *content;
};

/* Recebe uma string inteira pela rede
*/
int read_all(int sock, uint8_t *dst, int len);

/* Envia uma string inteira pela rede
*/
int write_all(int sock, uint8_t *buf, int len);

#endif
