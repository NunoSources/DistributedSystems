// Grupo 14
// Alexandre Torrinha fc52757
// Nuno Fontes fc46413
// Rodrigo Simões fc52758

#ifndef _TREE_PRIVATE_H
#define _TREE_PRIVATE_H

#include "tree.h"

struct tree_t {
	struct node *root;
};

struct node {
  struct entry_t *data;
	struct node *left;
  struct node *right;
};

#endif
