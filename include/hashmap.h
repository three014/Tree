#ifndef __HASHMAP_H
#define __HASHMAP_H

#include <stddef.h>

typedef struct hashmap_t hashmap_t;

hashmap_t *hashmap_new(void);

/// Frees all memory allocated to the hashmap, optionally freeing
/// the values in the hashmap as well.
///
/// The function pointer follows this convention:
///     void fn_name(void *ptr_to_value);
/// The function can assume that all pointers that are
/// passed-in are non-null.
void hashmap_delete(hashmap_t *map, void (*val_free)(void *val));

void *hashmap_get(hashmap_t *map, size_t key);
int hashmap_insert(hashmap_t *map, size_t key, void *val);
void *hashmap_remove(hashmap_t *map, size_t key);
int hashmap_is_empty(hashmap_t *map);

#endif

