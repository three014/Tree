#ifndef __ARENA_H
#define __ARENA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct arena_t arena_t;
typedef struct arena_temp_t arena_temp_t;

/// Allocates memory for an arena
/// and returns a pointer to it.
arena_t *arena_new(void);

/// Resets the arena's buffer offset
/// to 0, effectively clearing out
/// the internal buffer.
void arena_clear(arena_t *arena);

/// Allocates `size` bytes within the
/// arena and returns a pointer to the
/// beginning of that block of memory.
///
/// Accessing memory outside the size given
/// is undefined behavior. Furthermore,
/// calling this function while a temp arena
/// for this arena is active will return a
/// NULL pointer.
void *arena_alloc(arena_t *arena, const size_t size);

/// Completely frees all memory
/// associated with arena, including itself.
void arena_delete(arena_t *arena);

/// Creates a handle to an existing arena and
/// that arena's current offset. The returned
/// handle can be passed into `arena_temp_delete`
/// to reset the arena's offset to the value
/// saved in the handle.
///
/// While this and all future temp arenas
/// created from this handle are active, the main
/// arena will not be allowed to create its
/// own allocations, as those allocations won't
/// be permanent and would be deallocated upon
/// deallocating the temp arenas.
arena_temp_t *arena_temp_new(arena_t *arena);

/// Just like `arena_temp_new`, creates a handle
/// to an existing arena and its current offset.
/// However, keep track of these temporary arenas,
/// as freeing one temp will free all the temps
/// created after that one, AND all temps need
/// to be freed before an arena can be used to 
/// create persistent allocations again.
arena_temp_t *arena_temp_from_temp(arena_temp_t *temp);

/// Allocates `size` bytes on the arena that `temp`
/// originated from and returns a pointer to the 
/// start of the allocation.
void *arena_temp_alloc(arena_temp_t *temp, const size_t size);

/// Deletes a temp arena and returns the original arena's
/// offset to the one stored in this temp arena. If 
/// other temps were created after this temp, then
/// those temps and all data associated with them
/// will be deallocated as well.
///
/// Upon deleting the first created temp for the
/// given arena, the arena will be allowed to create 
/// persistent allocations again.
void arena_temp_delete(arena_temp_t *temp);

#ifdef __cplusplus
}
#endif

#endif // __ARENA_H