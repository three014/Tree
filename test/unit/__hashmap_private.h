#ifndef __HASHMAP_PRIVATE_H
#define __HASHMAP_PRIVATE_H

#include "../../include/hashmap.h"
#include <stdint.h>

size_t __calc_index(uint32_t seed, size_t key, size_t len);

#endif // !__HASHMAP_PRIVATE_H
