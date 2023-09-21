#include "string2.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_CAPACITY 16

int string_alloc(owned_string_t *str) {
    return string_alloc_with_cap(str, DEFAULT_CAPACITY);
}

int string_alloc_with_cap(owned_string_t *str, size_t capacity) {
    char *buf = malloc(sizeof(char) * capacity);
    if (!buf) {
        return 0;
    }

    (void) memset(buf, 0, capacity);
    (*str) = (owned_string_t) {
        .buf = buf,
        .cap = capacity,
        .len = capacity
    };

    return 1;
}

void string_delete(owned_string_t *str) {
    if (str->buf) {
        free(str->buf);
    }

    str->buf = NULL;
    str->cap = 0;
    str->len = 0;
}

borrowed_string_t string_view(owned_string_t *str) {
    return (borrowed_string_t) {
        .buf = str->buf,
        .len = str->len,
    };
}