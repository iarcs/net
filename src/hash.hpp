#pragma once

#include <cstddef>

namespace hash {

size_t hash(const void *data, size_t len);
size_t hash(const void *data, size_t len, size_t seed);
size_t hash_combine(size_t value, size_t seed);

} // namespace hash
