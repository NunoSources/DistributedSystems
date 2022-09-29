// Grupo 14
// Alexandre Torrinha fc52757
// Nuno Fontes fc46413
// Rodrigo Simões fc52758

#include "entry.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct data_t *data_create(int size) {
    if(size <= 0) {
        return NULL;
    }
    struct data_t *new_data = malloc(sizeof(struct data_t));
    new_data->datasize = size;
    new_data->data = malloc(size);
    return new_data;
}

struct data_t *data_create2(int size, void *data) {
    if(data == NULL || size <= 0) return NULL;
    struct data_t *new_data = malloc(sizeof(struct data_t));
    if(new_data == NULL) return NULL;
    new_data->datasize = size;
    new_data->data = data;
    return new_data;
}

void data_destroy(struct data_t *data) {
    if(data == NULL || data->data == NULL || data->datasize <= 0) return;
    free(data->data);
    free(data);
    return;
}

struct data_t *data_dup(struct data_t *data) {
    if(data == NULL || data->data == NULL || data->datasize <= 0) return NULL;
    void *data_copy = malloc(data->datasize);
    memcpy(data_copy, data->data, data->datasize);
    struct data_t *new_data = data_create2(data->datasize, data_copy);
    return new_data;
}

void data_replace(struct data_t *data, int new_size, void *new_data) {
    if(new_size <=0 || data->data == NULL || data->datasize <= 0 || new_data == NULL)
        return;
    free(data->data);
    data->data = new_data;
    data->datasize = new_size;
    return;
}
