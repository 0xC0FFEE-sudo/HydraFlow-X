/**
 * @file memory_pool.cpp
 * @brief Memory pool implementation (mainly template instantiations)
 */

#include "memory_pool.hpp"
#include <iostream>

namespace hfx::core {

// Explicit template instantiations for common types
template class MemoryPool<char>;
template class MemoryPool<int>;
template class MemoryPool<double>;

} // namespace hfx::core
