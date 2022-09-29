// Grupo 14
// Alexandre Torrinha fc52757
// Nuno Fontes fc46413
// Rodrigo Simões fc52758

// funções read_all e write_all que vão receber e enviar
// strings inteiras pela rede.
#include "message-private.h"

int write_all(int sock, uint8_t *buf, int len) {
  int buf_size = len;
  // Envia string
  while(len>0) {
    int res = write(sock, buf, len);
    if(res < 0) {
      if(errno == EINTR) continue;
      perror("write failed:");
      return res;
    }
    buf += res;
    len -= res;
  }
  return buf_size;
}

int read_all(int sock, uint8_t *dst, int len) {
  int buf_size = len;
  // Envia string
  while(buf_size>0) {
    int res = read(sock, dst, len);
    if(res < 0) {
      if(errno == EINTR) continue;
      perror("read failed:");
      return res;
    }
    dst += res;
    buf_size -= res;
  }
  return len;
}
