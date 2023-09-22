#ifndef __ALLOC_H
#define __ALLOC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *(*alloc)(const size_t n);
    void (*free)(void *ptr);
} allocator;

#ifdef __cplusplus
}
#endif

#endif