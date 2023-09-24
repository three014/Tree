#include "hashmap.h"
#include "error.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    size_t key;
    void *val;
} kv_t;

typedef struct {
    size_t len;
    size_t cap;
    kv_t *pairs;
} bucket_t;

typedef struct {
    size_t len;
    bucket_t *buf;
} container_t;

struct hashmap_t {
    int seed;
    container_t buckets;
};

// ----------------------------------------------------
// Murmur3 32-bit Hash Algorithm
// ----------------------------------------------------

static inline uint32_t murmur_32_scramble(uint32_t k) {
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}

uint32_t murmur3_32(const uint8_t *key, size_t len, uint32_t seed) {
    uint32_t h = seed;
    uint32_t k;

    /* Read in groups of 4. */
    for (size_t i = len >> 2; i; i--) {
        // Here is a source of differing results across endiannesses.
        // A swap here has no effects on hash properties though.
       (void) memcpy(&k, key, sizeof(uint32_t));
       key += sizeof(uint32_t);
       h ^= murmur_32_scramble(k);
       h = (h << 13) | (h >> 19);
       h = h * 5 + 0xe6546b64;
    }

    /* Read the rest. */
    k = 0;
    for (size_t i = len & 3; i; i--) {
        k <<= 8;
        k |= key[i - 1];
    }

    // A swap is *not* necessary here because the preceding loop already
    // places the low bytes in the low places according to whatever
    // endianness we use. Swaps only apply when the memory is copied in a chunk.
    h ^= murmur_32_scramble(k);

    /* Finalize. */
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

// ----------------------------------------------------
// Vector/Bucket Functions
// ----------------------------------------------------

inline int key_eq(kv_t *left, kv_t *right) {
    return left->key == right->key;
}

inline size_t max(size_t left, size_t right) {
    if (left >= right) {
        return left;
    }
    return right;
}

void bucket_reserve(bucket_t *bucket, size_t len) {
    size_t new_cap = bucket->len + 1;

    new_cap = max(bucket->cap * 2, new_cap);
#define CEILING 4
    new_cap = max(CEILING, new_cap);
#undef CEILING

    void *new_ptr = realloc(bucket->pairs, new_cap);
    if (!new_ptr) handle_error(strerror(errno));
    bucket->pairs = new_ptr;
    bucket->cap = new_cap;
}

void *bucket_remove(bucket_t *bucket, size_t idx) {
    if (bucket == NULL || idx >= bucket->len) return NULL;
    kv_t *ptr = bucket->pairs + (uintptr_t) idx;
    void *ret = ptr->val;
    (void) memmove(ptr, ptr + 1, bucket->len - idx - 1);
    bucket->len--;
    return ret;
}

void bucket_push(bucket_t *bucket, kv_t kv) {
    if (bucket->len == bucket->cap) {
        bucket_reserve(bucket, bucket->len);
    }

    kv_t *end = bucket->pairs + (uintptr_t) bucket->len;
    (*end) = kv;
    bucket->len++;
}

// ----------------------------------------------------
// Hashmap Function Definitions
// ----------------------------------------------------

hashmap_t *hashmap_new(void) {
    hashmap_t *map = malloc(sizeof(hashmap_t));
    if (!map) handle_error(strerror(errno));

    (*map) = (hashmap_t) {
        .seed = time(0),
        .buckets = {0}
    };

    return map;
}

void hashmap_delete(hashmap_t *map, void (*val_free)(void *val)) {
    if (!map) return;
    if (val_free != NULL) {
        for (size_t i = 0; i < map->buckets.len; i++) {
            bucket_t *bucket = map->buckets.buf + i;
            if (bucket->cap == 0) continue;
            for (size_t j = 0; j < bucket->len; j++) {
                val_free(bucket->pairs->val);
            }
            free(bucket->pairs);
        }
        free(map->buckets.buf);
    } else {
        for (size_t i = 0; i < map->buckets.len; i++) {
            bucket_t *bucket = map->buckets.buf + i;
            if (bucket->cap == 0) continue;
            free(bucket->pairs);
        }
        free(map->buckets.buf);
    }
    free(map);
}

void *hashmap_get(hashmap_t *map, size_t key);
int hashmap_insert(hashmap_t *map, size_t key, void *val);
void *hashmap_remove(hashmap_t *map, size_t key);
int hashmap_is_empty(hashmap_t *map);