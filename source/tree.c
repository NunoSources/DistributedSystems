// Grupo 14
// Alexandre Torrinha fc52757
// Nuno Fontes fc46413
// Rodrigo Simões fc52758

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "entry.h"
#include "data.h"
#include "tree.h"
#include "tree-private.h"

#define max(x,y) ((x) >= (y)) ? (x) : (y)

struct node *new_node(char *key, struct data_t *value) {
    struct node* new_node = malloc(sizeof(struct node));
    if (new_node == NULL) {
        return NULL;
    }
    new_node->data = entry_create(key, value);
    new_node->right = NULL;
    new_node->left = NULL;
    return new_node;
}

void node_destroy(struct node *node) {
    if (node != NULL) {
        node_destroy(node->right);
        node_destroy(node->left);
        if(node->data != NULL) {
            entry_destroy(node->data);
        }
        free(node);
    }
    return;
}

void tree_destroy(struct tree_t *tree) {
    if(tree != NULL) {
        node_destroy(tree->root);
        free(tree);
    }
    return;
}

struct tree_t *tree_create() {
    struct tree_t* new_tree = (struct tree_t*) malloc(sizeof(struct tree_t));
    if (new_tree == NULL) {
        return NULL;
    }
    new_tree->root = NULL;
    return new_tree;
}

struct node *node_put(struct node* root, char *key, struct data_t *value) {
    if(root == NULL) {
        return new_node(key, value);
    }
    int compareKeys = strcmp(key, root->data->key);
    if (compareKeys < 0) {
        root->left = node_put(root->left, key, value);
    } else if (compareKeys > 0) {
        root->right = node_put(root->right, key, value);
    } else if (compareKeys == 0) {
        entry_replace(root->data, key, value);
    }
    return root;
}

int tree_put(struct tree_t *tree, char *key, struct data_t *value) {
    //Cria copias da chave e do valor
    char *key_copy = strdup(key);

    if(key_copy == NULL) {
        return -1;
    }
    struct data_t *value_copy = data_dup(value);
    if(value_copy == NULL) {
        return -1;
    }

    tree->root = node_put(tree->root, key_copy, value_copy);
    return 0;
}

struct data_t *node_get(struct node *root, char *key) {
    if(root == NULL) {
        return NULL;
    }
    int compareKeys = strcmp(key, root->data->key);
    if (compareKeys < 0) {
        return node_get(root->left, key);
    } else if (compareKeys > 0){
        return node_get(root->right, key);
    }
    //When found (==0)
    return root->data->value;
}

struct data_t *tree_get(struct tree_t *tree, char *key) {
    struct data_t* data_found = node_get(tree->root, key);
    if(data_found == NULL) {
      return NULL;
    }
    struct data_t* data_copy = data_dup(data_found);
    return data_copy;
}

struct node *find_min_child(struct node *root) {
    if (root == NULL) {
        return NULL;
    } else if (root->left != NULL) {
        return find_min_child(root->left);
    }
    return root;
}

void inorder(struct node *root) {
    if(root!=NULL) // checking if the root is not null
    {
        inorder(root->left); // visiting left child
        printf(" %s ", root->data->key); // printing data at root
        inorder(root->right);// visiting right child
    }
}

void inorder_t(struct tree_t *tree) {
    inorder(tree->root);
}

struct node *node_del(struct node *root, char *key, int *found) {
    if (root == NULL) {
        *found = 1;
        return NULL;
    }

    int compareKeys = strcmp(key, root->data->key);
    if (compareKeys < 0) {
        root->left = node_del(root->left, key, found);
    } else if (compareKeys > 0) {
        root->right = node_del(root->right, key, found);
    } else { //Found key
        //No children
        if (root->left == NULL && root->right == NULL) {
            free(root);
            return NULL;
        // One child
        } else if (root->left == NULL && root->right == NULL) {
            struct node *temp;
            if(root->left == NULL) {
                temp = root->right;
            } else {
                temp = root->left;
            }
            entry_destroy(root->data);
            free(root);
            return temp;
        } else {
            struct node *temp = find_min_child(root->right);
            root->data = temp->data;
            root->right = node_del(root->right, temp->data->key, found);
        }
    }
    return root;
}

int tree_del(struct tree_t *tree, char *key) {
    int *found = calloc(1, sizeof(int));

    struct node* result = NULL;
    result = node_del(tree->root, key, found);
    if (*found == 1) {
        free(found);
        return -1;
    }
    free(found);
    tree->root = result;
    return 0;
}

int node_size(struct node *node) {
    if (node == NULL) {
        return 0;
    }
    return 1 + node_size(node->left) + node_size(node->right);
}

int tree_size(struct tree_t *tree) {
    int n = node_size(tree->root);
    return n;
}

int node_height(struct node *node, int n) {
    if (node == NULL) {
        return n;
    }
    return max(node_height(node->left, n+1), node_height(node->right, n+1));
}

int tree_height(struct tree_t *tree) {
    return node_height(tree->root, 0);
}

int get_inorder(char **keys_list, struct node *node, int index) {
    if (node == NULL) {
        return index;
    }

    keys_list[index] = strdup(node->data->key);
    index++;
    if (node->left != NULL) {
        index = get_inorder(keys_list, node->left, index);
    }
    if (node->right != NULL) {
        index = get_inorder(keys_list, node->right, index);
    }
    return index;
}

char **tree_get_keys(struct tree_t *tree) {
    int tree_s = tree_size(tree);
    int allocsize = (tree_s + 1) * sizeof(char*);
    char **keys_list = malloc(allocsize);

    int index = 0;
    index = get_inorder(keys_list, tree->root, index);

    keys_list[tree_s] = NULL;
    return keys_list;
}

int get_array_size(char **keys) {
    int i = 0;

    while(keys[i] != NULL) {
        i++;
    }
    return i;
}

void tree_free_keys(char **keys) {
    int n = get_array_size(keys);
    for (int i=0; i < n; i++) {
        free(keys[i]);
    }
    free(keys);
    return;
}
