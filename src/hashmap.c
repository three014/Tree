#include "hashmap.h"
#include "error.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

typedef struct vector_t {
    size_t len;
    size_t cap;
    bucket_t *buf;
} vector_t;

typedef struct bucket_t {
    size_t len;
    size_t cap;
    kv_t *pairs;
} bucket_t;

typedef struct {
    size_t key;
    void *val;
} kv_t;

struct hashmap_t {
    int seed;
    vector_t buf;
};

inline int key_eq(kv_t *left, kv_t *right) {
    return left->key == right->key;
}

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
// Hashmap Function Definitions
// ----------------------------------------------------

hashmap_t *hashmap_new(void) {
    hashmap_t *map = malloc(sizeof(hashmap_t));
    if (!map) {
        dbg_line_no_fmt("Can't allocate memory for hashmap!\n");
        return NULL;
    }

    (*map) = (hashmap_t) {
        .seed = time(0),
        .buf = {0}
    };

    return map;
}


void hashmap_delete(hashmap_t *map);
void *hashmap_get(hashmap_t *map, size_t key);
int hashmap_insert(hashmap_t *map, size_t key, void *val);
void *hashmap_remove(hashmap_t *map, size_t key);
int hashmap_is_empty(hashmap_t *map);