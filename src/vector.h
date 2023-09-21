#ifndef __VECTOR_H
#define __VECTOR_H

#include <stddef.h>

typedef struct vector_t vector_t;

vector_t *vector_new(void);
void vector_delete(vector_t *vector);
void *vector_get(vector_t *vector, size_t idx);

#endif