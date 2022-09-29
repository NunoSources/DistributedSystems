// Grupo 14
// Alexandre Torrinha fc52757
// Nuno Fontes fc46413
// Rodrigo Simões fc52758

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "data.h"
#include "entry.h"

struct entry_t *entry_create(char *key, struct data_t *data){
    if(key == NULL || data == NULL) {
        return NULL;
    }
    struct entry_t *entry = malloc(sizeof(struct entry_t));
    if(entry == NULL){
        perror("Error: memory not allocated");
        return NULL;
    }
    entry->key = key;
    entry->value = data;
    return entry;
}

void entry_initialize(struct entry_t *entry){
    if(entry == NULL){
        perror("Error: wrong argument");
        return;
    }
    /*initialize entry*/
    entry->key = NULL;
    entry->value = NULL;
}

void entry_destroy(struct entry_t *entry){
    if(entry == NULL){
        perror("Error: wrong argument");
        return;
    }
    /*free memory*/
    if(entry->key != NULL){
        free(entry->key);
    }
    if(entry->value != NULL){
        data_destroy(entry->value);
    }
    free(entry);
}

struct entry_t *entry_dup(struct entry_t *entry){
    struct entry_t *other = malloc(sizeof(struct entry_t));
    if(entry == NULL){
        perror("Error: wrong argument");
        return NULL;
    }
    /*alloc other entry memory*/
    /*char *key; string, cadeia de caracteres terminada por '\0' */
    if((other->key = malloc(strlen(entry->key) + 1)) == NULL){
    free(other);
        perror("Error: memory not allocated");
        return NULL;
    }
    strcpy(other->key, entry->key);
    if((other->value = data_dup(entry->value)) == NULL){
        free(other);
        return NULL;
    }
    return other;
}

void entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value){
    if(entry == NULL){
        perror("Error: wrong argument");
        return;
    }
    free(entry->key);
    data_destroy(entry->value);
    entry->key = new_key;
    entry->value = new_value;
}

int entry_compare(struct entry_t *entry1, struct entry_t *entry2){
    int result = strcmp(entry1->key, entry2->key);
    if(result == 0){
        result = 0;
    }else if(result < 0){
        result = -1;
    }else{
        result = 1;
    }
    return result;
}
