#ifndef __STRING2_H
#define __STRING2_H

#include <stddef.h>

typedef struct {
    size_t len;
    size_t cap;
    char *buf;
} owned_string_t;

typedef struct {
    size_t len;
    char *buf;
} borrowed_string_t;

int string_alloc(owned_string_t *str);
int string_alloc_with_cap(owned_string_t *str, size_t capacity);
void string_delete(owned_string_t *str);

borrowed_string_t string_view(owned_string_t *str);

#endif