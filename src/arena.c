#define _GNU_SOURCE

#include "arena.h"
#include "log.h"
#include "hashmap.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

struct arena_t {
    size_t offset;
    const size_t page_size;
    size_t num_pages;
    arena_temp_t *first;
    arena_temp_t *last;
    void *buf;
};

struct arena_temp_t {
    const size_t saved_offset;
    arena_t *arena;
    arena_temp_t *next;
};

/// The result of attempting
/// to allocate memory from this
/// arena.
enum AllocResult {
    /// The allocation succeeded.
    ALLOC_SUCCESS = 0,

    /// Allocation failed - The total
    /// amount of memory allocated
    /// exceeded the maximum amount
    /// of memory that was allowed
    /// to be allocated.
    OUT_OF_VIRT = 1,

    /// Allocation falied - The
    /// arena requested more memory
    /// and that request failed (check errno).
    ALLOC_FAILED = 2,
};

#if INTPTR_MAX == INT64_MAX
#define MAX_ALLOC_SPACE 34359738368 // 32 Gigabytes
#elif INTPTR_MAX == INT32_MAX
#define MAX_ALLOC_SPACE 1073741824 // 1 Gigabyte
#else
#error Unknown pointer size or missing size macros!
#endif

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif


// -------------------------------------------------
// GLOBAL ARENA COLLECTION
// -------------------------------------------------

static volatile hashmap_t *thread_arenas = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int global_exists(void) {
    hashmap_t *global = (hashmap_t *)thread_arenas;
    return global != NULL;
}

/// Returns a pointer to the global arena collection.
/// If it doesn't exist, this function initializes
/// it and returns a pointer to that allocation.
hashmap_t *global_get(void) {
    hashmap_t *global = (hashmap_t *)thread_arenas;
    if (global == NULL) {
        pthread_mutex_lock(&mutex);
        global = (hashmap_t *)thread_arenas;
        if (global == NULL) {
            global = hashmap_new();
            thread_arenas = global;
        }
        pthread_mutex_unlock(&mutex);
    }
    return global;
}

void global_free(void) {
    hashmap_t *global = (hashmap_t *)thread_arenas;
    if (global != NULL) {
        pthread_mutex_lock(&mutex);
        global = (hashmap_t *)thread_arenas;
        if (global != NULL) {
            // HASHMAP MUST BE EMPTY HERE OR ELSE 
            // THIS WILL LEAK ARENA ALLOCATORS
            hashmap_delete(global, NULL);
            thread_arenas = NULL;
        }
        pthread_mutex_unlock(&mutex);
    }
}

int global_insert(arena_t *arena) {
    hashmap_t *global = global_get();
    pthread_t self = pthread_self();
    pthread_mutex_lock(&mutex);
    int success = hashmap_insert(global, (size_t)self, (void *)arena);
    pthread_mutex_unlock(&mutex);
    return success;
}

arena_t *global_remove(void) {
    if (!global_exists()) return NULL;
    hashmap_t *global = global_get();
    pthread_t self = pthread_self();
    pthread_mutex_lock(&mutex);
    arena_t *arena = hashmap_remove(global, (size_t)self);
    pthread_mutex_unlock(&mutex);
    return arena;
}

arena_t *global_view(void) {
    if (!global_exists()) return NULL;
    hashmap_t *global = global_get();
    pthread_t self = pthread_self();
    pthread_mutex_lock(&mutex);
    arena_t *arena = hashmap_get(global, (size_t)self);
    pthread_mutex_unlock(&mutex);
    return arena;
}

int global_is_empty(void) {
    if (!global_exists()) return 1;
    hashmap_t *global = global_get();
    pthread_mutex_lock(&mutex);
    int empty = hashmap_is_empty(global);
    pthread_mutex_unlock(&mutex);
    return empty;
}


// -------------------------------------------------
// ARENA HELPER FUNCTIONS
// -------------------------------------------------

/// Helper function to the `arena_new` function,
/// reserves the page-aligned amount of virtual
/// memory as defined by MAX_ALLOC_SPACE, but does
/// not actually reserve any physical memory.
///
/// Returns a pointer to the start of the new
/// mapping, or NULL if the mapping failed.
/// It is recommended to handle this failure
/// by exiting out of the program.
static void *__reserve_mem(const size_t pagesize);

/// Attempts to map a new page of physical memory using mmap(). If
/// successful, returns ALLOC_SUCCESS, otherwise the mapping failed
/// and the result should be handled.
static enum AllocResult __map_new_page(arena_t *arena, const uintptr_t new_arena_size);

/// Aligns the address with the specified alignment and returns the
/// new address that the next allocation should start at.
static uintptr_t align(const uintptr_t ptr, const size_t alignment);

/// Returns 1 if true, 0 if false.
inline int is_power_of_two(const uintptr_t x);

/// Attempts to allocate memory with the given arena, but will
/// return NULL if the arena has any temp arenas attached to itself.
///
/// This function will also error out of the program if the given
/// allocator is NULL.
void *alloc_checked(arena_t *arena, const size_t size);

/// Attempts to allocate memory without checking if the given
/// arena has any temp arenas attached to itself.
void *alloc_unchecked(arena_t *arena, const size_t size);

/// Compares two temp arenas and returns 1 if they are the same.
/// Otherwise returns 0.
int temp_arena_eq(const arena_temp_t *self, const arena_temp_t *other);

void print_linked_list(arena_temp_t *start);
void print_arena_temp(const arena_temp_t *temp);
void print_arena_info(const arena_t *arena);
void print_temp_info(const arena_temp_t *temp);
void print_arena(const arena_t *arena);

/// Allocates memory for an arena
/// and returns a pointer to it.
arena_t *arena_new(void);


// --------------------------------------------------------------
// ARENA ALLOCATOR DEFINITIONS
// --------------------------------------------------------------

arena_t *arena_new(void) {
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (page_size == -1) {
        __logln_err_fmt("Sysconf: %s", strerror(errno));
        exit(1);
    }

    void *non_committed_addr = __reserve_mem(page_size);
    if (non_committed_addr == MAP_FAILED) {
        __logln_err_fmt("Failed to reserve memory for arena: %s", strerror(errno));
        exit(1);
    }

    void *addr = mmap(non_committed_addr, page_size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

    if (addr == MAP_FAILED) {
        __logln_warn_fmt("%s", strerror(errno));
        return NULL;
    }

    arena_t arena = {
        .buf = (void *)((uintptr_t) addr + sizeof(arena_t)),
        .num_pages = 1,
        .offset = 0,
        .first = NULL,
        .last = NULL,
        .page_size = page_size
    };

    (void) memcpy(addr, &arena, sizeof(arena_t));

    int success = global_insert((arena_t *) addr);
    if (!success) {
        __logln_err("There currently can't be more than one arena per thread");
        exit(1);
    }

    return (arena_t *)addr;
}

static void *__reserve_mem(const size_t page_size) {
    size_t max_alloc_space = MAX_ALLOC_SPACE;
    size_t max_num_pages = max_alloc_space / page_size;
    size_t total_page_size = max_num_pages * page_size;

    dbg("[%s:%lu] total_page_size = %lu\n", __FILE__, __LINE__,
        total_page_size);

    return mmap(NULL, total_page_size, PROT_NONE,
                MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE, -1, 0);
}

void *arena_alloc(const size_t size) {
    arena_t *arena = global_view();
    if (arena == NULL) {
        arena = arena_new();
    }
    return alloc_checked(arena, size);
}

void *alloc_checked(arena_t *arena, const size_t size) {
    assert(arena != NULL);

    if (arena->last == NULL)
        return alloc_unchecked(arena, size);
    else
        return NULL;
}

void *alloc_unchecked(arena_t *arena, const size_t size) {
    const uintptr_t cur_addr = (uintptr_t)arena->buf + (uintptr_t)arena->offset;
    const uintptr_t offset = align(cur_addr, DEFAULT_ALIGNMENT);
    const uintptr_t arena_size = offset - (uintptr_t)arena;

    if (arena_size + size >= arena->page_size * arena->num_pages) {
        switch (__map_new_page(arena, arena_size)) {
        case OUT_OF_VIRT:
            // Error out, we hit the max
            __logln_err_fmt("%s", strerror(errno));
            exit(1);
        case ALLOC_FAILED:
            __logln_warn("Could not map a new page");
            return NULL;
        case ALLOC_SUCCESS:
            arena->num_pages++;
        }
    }

    const uintptr_t relative_offset = offset - (uintptr_t)arena->buf;
    void *ret = (void *)((uintptr_t)arena->buf + relative_offset);
    arena->offset = relative_offset + size;

    (void)memset(ret, 0, size);

    return ret;
}

static uintptr_t align(const uintptr_t ptr, const size_t alignment) {
    assert(is_power_of_two(alignment));

    uintptr_t ret = ptr;
    uintptr_t a = (uintptr_t)alignment;
    uintptr_t modulo = ret & (a - 1);

    if (modulo != 0) {
        // If 'ret' address is not aligned, push the address
        // to the next value which is aligned.
        ret += a - modulo;
    }

    return ret;
}

inline int is_power_of_two(const uintptr_t x) { return (x & (x - 1)) == 0; }

static enum AllocResult __map_new_page(arena_t *arena, const uintptr_t new_arena_size) {
    const uintptr_t max_alloc_space = MAX_ALLOC_SPACE;
    if (new_arena_size >= max_alloc_space - arena->page_size) {
        return OUT_OF_VIRT;
    }

    void *next_addr = arena + arena->page_size;

    next_addr = mmap(next_addr, arena->page_size + 1, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

    if (next_addr == MAP_FAILED) {
        __logln_err_fmt("%s", strerror(errno));
        return ALLOC_FAILED;
    }

    return ALLOC_SUCCESS;
}

void arena_delete(void) {
    arena_t *arena = global_remove();
    if (arena == NULL) return;
    if (munmap(arena, MAX_ALLOC_SPACE) == -1) {
        dbg("%s\n", strerror(errno));
    }

    if (global_is_empty()) {
        global_free();
    }
}

void arena_clear(void) {
    arena_t *arena = global_view();
    if (arena != NULL) {
        arena->offset = 0;
        arena->first = NULL;
        arena->last = NULL;
    }
}

arena_temp_t *arena_temp_new(void) {
    arena_t *arena = global_view();
    if (arena == NULL) {
        arena = arena_new();
    }

    arena_temp_t tmp = {
        .arena = arena, 
        .saved_offset = arena->offset, 
        .next = NULL
    };

    arena_temp_t *temp_arena = alloc_unchecked(arena, sizeof(arena_temp_t));
    if (temp_arena != NULL) {
        (void) memcpy(temp_arena, &tmp, sizeof(arena_temp_t));
        arena_temp_t *last = arena->last;
        if (last != NULL) {
            last->next = temp_arena;
        } else {
            arena->first = temp_arena;
        }
        arena->last = temp_arena;
    }

    return temp_arena;
}

void *arena_temp_alloc(arena_temp_t *temp, const size_t size) {
    return alloc_unchecked(temp->arena, size);
}

void arena_temp_delete(arena_temp_t *temp) {
    arena_t *arena = temp->arena;
    print_arena(arena);
    print_linked_list(arena->first);
    arena_temp_t *visitor = arena->first;
    if (visitor == NULL || temp == NULL)
        return;

    if (temp_arena_eq(visitor, temp) == 1) {
        arena->first = NULL;
        arena->last = NULL;
    }

    arena_temp_t *visitor_next = visitor->next;
    while (visitor_next != NULL) {
        if (temp_arena_eq(visitor_next, temp) == 1) {
            visitor->next = NULL;
            break;
        }

        visitor = visitor_next;
        visitor_next = visitor_next->next;
    }

    arena->offset = temp->saved_offset;

    print_arena(arena);
    print_linked_list(arena->first);
}

int temp_arena_eq(const arena_temp_t *self, const arena_temp_t *other) {
    if (self->saved_offset == other->saved_offset)
        return 1;
    else
        return 0;
}

void print_linked_list(arena_temp_t *start) {
    arena_temp_t *visitor = start;
    while (visitor != NULL) {
        print_arena_temp(visitor);
        visitor = visitor->next;
    }
}

void print_arena(const arena_t *arena) {
    dbg("[%s:%u] arena_t [%p] {\n", __FILE__, __LINE__, arena);
    dbg("  .offset = %lu\n", arena->offset);
    dbg("  .page_size = %lu\n", arena->page_size);
    dbg("  .num_pages = %lu\n", arena->num_pages);
    dbg("  .first = ");
    print_temp_info(arena->first);
    dbg(",\n");
    dbg("  .last = ");
    print_temp_info(arena->last);
    dbg(",\n");
    dbg("  .buf = (void *) [%p]\n", arena->buf);
    dbg("}\n");
}

void print_arena_temp(const arena_temp_t *temp) {
    dbg("[%s:%u] arena_temp_t [%p] {\n", __FILE__, __LINE__, temp);
    dbg("  .saved_offset = %lu,\n", temp->saved_offset);
    dbg("  .arena = ");
    print_arena_info(temp->arena);
    dbg(",\n");
    dbg("  .next = ");
    print_temp_info(temp->next);
    dbg("\n");
    dbg("}\n");
}

void print_arena_info(const arena_t *arena) {
    if (arena != NULL) {
        dbg("arena_t [%p]", arena);
    } else {
        dbg("NULL");
    }
}

void print_temp_info(const arena_temp_t *temp) {
    if (temp != NULL) {
        dbg("arena_temp_t [%p]", temp);
    } else {
        dbg("NULL");
    }
}

