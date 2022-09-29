#include "zookeeper/zookeeper.h"

struct rtree_t {
  char* address;
  char* port;
  int socketfd;
};
