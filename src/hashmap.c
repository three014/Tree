#include "hashmap.h"
#include "error.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define __TRUE 1
#define __FALSE 0
#define __CEILING 4

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
    size_t size;
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
        (void)memcpy(&k, key, sizeof(uint32_t));
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

static void __bucket_reserve(bucket_t *bucket, size_t len) {
    size_t new_cap = bucket->len + 1;

    new_cap = max(bucket->cap * 2, new_cap);
    new_cap = max(__CEILING, new_cap);

    void *new_ptr = realloc(bucket->pairs, sizeof(kv_t) * new_cap);
    if (!new_ptr) handle_error(strerror(errno));
    bucket->pairs = new_ptr;
    bucket->cap = new_cap;
}

void *bucket_remove(bucket_t *bucket, size_t idx) {
    if (bucket == NULL || idx >= bucket->len) return NULL;
    kv_t *ptr = bucket->pairs + (uintptr_t)idx;
    void *ret = ptr->val;
    (void)memmove(ptr, ptr + 1, bucket->len - idx - 1);
    bucket->len--;
    return ret;
}

void bucket_push(bucket_t *bucket, kv_t kv) {
    if (bucket->len == bucket->cap) {
        __bucket_reserve(bucket, bucket->len);
    }

    kv_t *end = bucket->pairs + (uintptr_t) bucket->len;
    (*end) = kv;
    bucket->len++;
}

/// Creates a new empty container table that has double the
/// length of the container that was passed in.
///
/// Does nothing to the original container. Returns a
/// container struct with the parameters of the grown table,
/// with the buffer already allocated.
/// Note that the bucket's buffers have not been allocated,
/// and their lengths and capacities have been set to zero.
/// That is so the user can rehash the old containers elements
/// and add them to the new bucket however they choose.
static container_t __container_grow(const container_t *container) {
    size_t new_len = container->len + 1;
    new_len = max(container->len * 2, new_len);
    new_len = max(__CEILING, new_len);

    size_t new_len_u8 = sizeof(bucket_t) * new_len;
    void *new_ptr = malloc(new_len_u8);
    if (!new_ptr) handle_error(strerror(errno));
    (void)memset(new_ptr, 0, new_len_u8);

    return (container_t) {
        .size = container->size,
        .len = new_len,
        .buf = new_ptr
    };
}

/// Frees the container and the underlying buckets, but
/// purposely forgets to free the memory of the values
/// within the key/value pairs. Use if rehashing, or
/// if the container is already empty.
static void __container_delete_nofree(container_t *container) {
    if (!container) return;

    for (size_t i = 0; i < container->len; i++) {
        bucket_t *bucket = container->buf + i;
        if (bucket->cap == 0) continue;
        free(bucket->pairs);
    }
    free(container->buf);

    (*container) = (container_t) {
        .size = 0,
        .len = 0,
        .buf = NULL
    };
}

/// Same as `__container_delete_nofree`, but iterates through each bucket
/// and calls `val_free` on each value from the key/value pair.
static void __container_delete_andfree(container_t *container, void (*val_free)(void *val)) {
    if (!container) return;

    for (size_t i = 0; i < container->len; i++) {
        bucket_t *bucket = container->buf + i;
        if (bucket->cap == 0) continue;
        for (size_t j = 0; j < bucket->len; j++) {
            val_free(bucket->pairs->val);
        }
        free(bucket->pairs);
    }
    free(container->buf);

    (*container) = (container_t) {
        .size = 0,
        .len = 0,
        .buf = NULL
    };
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
        __container_delete_andfree(&map->buckets, val_free);
    } else {
        __container_delete_nofree(&map->buckets);
    }
    free(map);
}

static void __hashmap_rehash(hashmap_t *map) {
    container_t new_buckets = __container_grow(&map->buckets);

    for (size_t i = 0; i < map->buckets.len; i++) {
        bucket_t *bucket = map->buckets.buf + i;
        if (bucket->cap == 0) continue;
        for (size_t j = 0; j < bucket->len; j++) {
            kv_t *pair = bucket->pairs + j;

            // Rehash key
            size_t hashed_key = murmur3_32(
                (const uint8_t *)&pair->key,
                sizeof(size_t),
                map->seed
            );

            // Determine bucket to insert key/value pair
            size_t index = hashed_key % new_buckets.len;
            bucket_t *new_bucket = new_buckets.buf + index;

            // Note: This will incur a new allocation
            // for every new bucket we push a pair into.
            // It might be nice to allocate memory for each
            // bucket ahead of time, but currently idk how.
            bucket_push(new_bucket, *pair);
        }
    }

    // Delete old container
    __container_delete_nofree(&map->buckets);

    // Replace with new container
    map->buckets = new_buckets;
}

void *hashmap_get(hashmap_t *map, size_t key) {
    if (!map || !map->buckets.buf) return NULL;

    // Get bucket from hashed key
    size_t hashed_key = murmur3_32(
        (const uint8_t *)&key,
        sizeof(size_t),
        map->seed
    );

    // Determine bucket to locate key/value pair
    size_t index = hashed_key % map->buckets.len;
    bucket_t *bucket = map->buckets.buf + index;

    // Linearly search for matching key, if it exists
    if (bucket->cap == 0) return NULL;
    for (size_t i = 0; i < bucket->len; i++) {
        kv_t *pair = bucket->pairs + i;
        if (pair->key == key) {
            return pair->val;
        }
    }

    // Key not found
    return NULL;
}

int hashmap_insert(hashmap_t *map, size_t key, void *val) {
    if (!map) return __FALSE;

    // Get bucket from hashed key
    if (map->buckets.len <= map->buckets.size * 4) {
        __hashmap_rehash(map); // EXPENSIVE
    }

    // Hash key
    size_t hashed_key = murmur3_32(
        (const uint8_t *)&key,
        sizeof(size_t),
        map->seed
    );

    // Determine bucket to insert key/value pair
    size_t index = hashed_key % map->buckets.len;
    bucket_t *bucket = map->buckets.buf + index;
    bucket_push(bucket, (kv_t) {.key = key, .val = val});

    // Update total size
    map->buckets.size++;

    return __TRUE;
}

void *hashmap_remove(hashmap_t *map, size_t key) {
    return NULL;
}

int hashmap_is_empty(hashmap_t *map) {
    return (map) ? map->buckets.size == 0 : __TRUE;
}
