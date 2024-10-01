#include "hash.hpp"

#define XXH_INLINE_ALL
#define XXH_STATIC_LINKING_ONLY
#include <xxhash.h>

namespace hash {

size_t hash(const void *data, size_t len) {
    return XXH3_64bits(data, len);
}

size_t hash(const void *data, size_t len, size_t seed) {
    return XXH3_64bits_withSeed(data, len, seed);
}

size_t hash_combine(size_t value, size_t seed) {
    return XXH3_64bits_withSeed(&value, sizeof(size_t), seed);
}

} // namespace hash
