#ifndef __HASHMAP_H
#define __HASHMAP_H

#include <stddef.h>

typedef struct hashmap_t hashmap_t;

hashmap_t *hashmap_new(void);
void hashmap_delete(hashmap_t *map, void (*val_free)(void *val));
void *hashmap_get(hashmap_t *map, size_t key);
int hashmap_insert(hashmap_t *map, size_t key, void *val);
void *hashmap_remove(hashmap_t *map, size_t key);
int hashmap_is_empty(hashmap_t *map);

#endif