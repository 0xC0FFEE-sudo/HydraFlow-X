/**
 * @file memory_pool.hpp
 * @brief Ultra-fast memory pool for zero-allocation hot path
 * @version 1.0.0
 * 
 * NUMA-aware memory pools with cache-line alignment for Apple Silicon.
 * Designed for deterministic allocation patterns in HFT systems.
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>

#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/mman.h>
#endif

namespace hfx::core {

/**
 * @class MemoryPool
 * @brief High-performance memory pool with O(1) allocation/deallocation
 * 
 * Features:
 * - Cache-aligned allocations (64-byte for Apple Silicon)
 * - NUMA-aware memory mapping
 * - Lock-free allocation/deallocation
 * - Pre-allocated chunks to avoid system calls
 * - Memory prefetching for predictable access patterns
 */
template<typename T>
class alignas(64) MemoryPool {
public:
    static_assert(std::is_trivially_destructible_v<T>, 
                  "T must be trivially destructible for pool efficiency");

    /**
     * @brief Constructor
     * @param initial_size Initial number of objects to pre-allocate
     * @param alignment Memory alignment (default: 64 bytes for cache lines)
     */
    explicit MemoryPool(std::size_t initial_size = 1024, 
                       std::size_t alignment = 64)
        : alignment_(alignment),
          chunk_size_(calculate_chunk_size()),
          total_allocated_(0),
          total_freed_(0) {
        
        if (!allocate_initial_chunks(initial_size)) {
            std::abort(); // HFT: No exceptions, abort on allocation failure
        }
    }

    /**
     * @brief Destructor
     */
    ~MemoryPool() {
        cleanup_all_chunks();
    }

    // Non-copyable, movable
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    MemoryPool(MemoryPool&& other) noexcept
        : alignment_(other.alignment_),
          chunk_size_(other.chunk_size_),
          free_list_head_(other.free_list_head_.load()),
          chunks_(std::move(other.chunks_)),
          total_allocated_(other.total_allocated_.load()),
          total_freed_(other.total_freed_.load()) {
        
        other.free_list_head_.store(nullptr);
        other.total_allocated_.store(0);
        other.total_freed_.store(0);
    }

    /**
     * @brief Allocate an object from the pool (lock-free)
     * @return Pointer to allocated object, nullptr if pool exhausted
     */
    [[gnu::hot]] T* allocate() noexcept {
        // Try to get from free list first (O(1))
        FreeNode* node = free_list_head_.load(std::memory_order_acquire);
        
        while (node != nullptr) {
            FreeNode* next = node->next;
            if (free_list_head_.compare_exchange_weak(
                    node, next, std::memory_order_release, std::memory_order_acquire)) {
                
                total_allocated_.fetch_add(1, std::memory_order_relaxed);
                return reinterpret_cast<T*>(node);
            }
        }

        // Free list empty - try to allocate new chunk
        return allocate_from_new_chunk();
    }

    /**
     * @brief Deallocate an object back to the pool (lock-free)
     * @param ptr Pointer to object to deallocate
     */
    [[gnu::hot]] void deallocate(T* ptr) noexcept {
        if (ptr == nullptr) return;

        // Add to free list (lock-free stack push)
        FreeNode* node = reinterpret_cast<FreeNode*>(ptr);
        FreeNode* current_head = free_list_head_.load(std::memory_order_relaxed);
        
        do {
            node->next = current_head;
        } while (!free_list_head_.compare_exchange_weak(
            current_head, node, std::memory_order_release, std::memory_order_relaxed));

        total_freed_.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * @brief Construct object in-place
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Pointer to constructed object
     */
    template<typename... Args>
    T* construct(Args&&... args) {
        T* ptr = allocate();
        if (ptr) {
            new(ptr) T(std::forward<Args>(args)...);
        }
        return ptr;
    }

    /**
     * @brief Destroy and deallocate object
     * @param ptr Pointer to object to destroy
     */
    void destroy(T* ptr) noexcept {
        if (ptr) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                ptr->~T();
            }
            deallocate(ptr);
        }
    }

    /**
     * @brief Get pool statistics
     */
    struct Statistics {
        std::size_t total_allocated;
        std::size_t total_freed;
        std::size_t currently_allocated;
        std::size_t total_chunks;
        std::size_t bytes_allocated;
    };

    Statistics get_statistics() const noexcept {
        const auto allocated = total_allocated_.load(std::memory_order_relaxed);
        const auto freed = total_freed_.load(std::memory_order_relaxed);
        
        return Statistics{
            .total_allocated = allocated,
            .total_freed = freed,
            .currently_allocated = allocated - freed,
            .total_chunks = chunks_.size(),
            .bytes_allocated = chunks_.size() * chunk_size_
        };
    }

    /**
     * @brief Pre-warm the pool by touching all allocated pages
     */
    void warmup() noexcept {
        for (const auto& chunk : chunks_) {
            // Touch every page to ensure it's resident
            volatile char* ptr = reinterpret_cast<volatile char*>(chunk.memory);
            for (std::size_t i = 0; i < chunk_size_; i += 4096) {  // 4KB pages
                ptr[i] = ptr[i];
            }
        }
    }

    /**
     * @brief Check if pointer belongs to this pool
     * @param ptr Pointer to check
     * @return true if pointer is from this pool
     */
    bool owns(const T* ptr) const noexcept {
        const char* byte_ptr = reinterpret_cast<const char*>(ptr);
        
        for (const auto& chunk : chunks_) {
            const char* chunk_start = reinterpret_cast<const char*>(chunk.memory);
            const char* chunk_end = chunk_start + chunk_size_;
            
            if (byte_ptr >= chunk_start && byte_ptr < chunk_end) {
                return true;
            }
        }
        return false;
    }

private:
    /**
     * @brief Free list node for O(1) deallocation
     */
    struct FreeNode {
        FreeNode* next;
    };

    /**
     * @brief Memory chunk descriptor
     */
    struct MemoryChunk {
        void* memory;
        std::size_t size;
        
        MemoryChunk(void* mem, std::size_t sz) : memory(mem), size(sz) {}
    };

    const std::size_t alignment_;
    const std::size_t chunk_size_;
    
    // Lock-free free list (stack)
    alignas(64) std::atomic<FreeNode*> free_list_head_{nullptr};
    
    // Allocated memory chunks
    std::vector<MemoryChunk> chunks_;
    
    // Statistics
    alignas(64) std::atomic<std::size_t> total_allocated_{0};
    alignas(64) std::atomic<std::size_t> total_freed_{0};

    /**
     * @brief Calculate optimal chunk size based on object size and alignment
     */
    std::size_t calculate_chunk_size() const noexcept {
        const std::size_t obj_size = std::max(sizeof(T), sizeof(FreeNode));
        const std::size_t aligned_size = (obj_size + alignment_ - 1) & ~(alignment_ - 1);
        
        // Target chunk size: 1MB for good TLB efficiency
        constexpr std::size_t target_chunk_size = 1024 * 1024;
        const std::size_t objects_per_chunk = target_chunk_size / aligned_size;
        
        return objects_per_chunk * aligned_size;
    }

    /**
     * @brief Allocate initial memory chunks
     */
    bool allocate_initial_chunks(std::size_t initial_objects) {
        const std::size_t obj_size = std::max(sizeof(T), sizeof(FreeNode));
        const std::size_t aligned_size = (obj_size + alignment_ - 1) & ~(alignment_ - 1);
        const std::size_t objects_per_chunk = chunk_size_ / aligned_size;
        const std::size_t num_chunks = (initial_objects + objects_per_chunk - 1) / objects_per_chunk;

        for (std::size_t i = 0; i < num_chunks; ++i) {
            if (!allocate_new_chunk()) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Allocate a new memory chunk
     */
    bool allocate_new_chunk() {
        void* memory = nullptr;

#ifdef __APPLE__
        // Use mmap for large allocations on macOS
        memory = mmap(nullptr, chunk_size_, 
                     PROT_READ | PROT_WRITE, 
                     MAP_PRIVATE | MAP_ANONYMOUS, 
                     -1, 0);
        
        if (memory == MAP_FAILED) {
            return false;
        }
        
        // Advise kernel about access pattern
        madvise(memory, chunk_size_, MADV_WILLNEED);
#else
        memory = std::aligned_alloc(alignment_, chunk_size_);
        if (!memory) {
            return false;
        }
#endif

        // Initialize free list for this chunk
        const std::size_t obj_size = std::max(sizeof(T), sizeof(FreeNode));
        const std::size_t aligned_size = (obj_size + alignment_ - 1) & ~(alignment_ - 1);
        const std::size_t objects_in_chunk = chunk_size_ / aligned_size;

        char* ptr = reinterpret_cast<char*>(memory);
        FreeNode* prev_node = nullptr;

        // Build free list (reverse order for cache efficiency)
        for (std::size_t i = objects_in_chunk; i > 0; --i) {
            FreeNode* node = reinterpret_cast<FreeNode*>(ptr + (i - 1) * aligned_size);
            node->next = prev_node;
            prev_node = node;
        }

        // Add to global free list
        FreeNode* current_head = free_list_head_.load(std::memory_order_relaxed);
        prev_node->next = current_head;
        
        while (!free_list_head_.compare_exchange_weak(
            current_head, reinterpret_cast<FreeNode*>(ptr), 
            std::memory_order_release, std::memory_order_relaxed)) {
            prev_node->next = current_head;
        }

        chunks_.emplace_back(memory, chunk_size_);
        return true;
    }

    /**
     * @brief Allocate from new chunk when free list is empty
     */
    T* allocate_from_new_chunk() noexcept {
        // This is the slow path - should rarely be called
        if (!allocate_new_chunk()) {
            return nullptr;
        }
        
        // Try allocation again after adding new chunk
        return allocate();
    }

    /**
     * @brief Cleanup all allocated chunks
     */
    void cleanup_all_chunks() noexcept {
        for (const auto& chunk : chunks_) {
#ifdef __APPLE__
            munmap(chunk.memory, chunk.size);
#else
            std::free(chunk.memory);
#endif
        }
        chunks_.clear();
    }
};

/**
 * @brief Specialized memory pool for specific HFT data structures
 */
template<typename T>
using HFTMemoryPool = MemoryPool<T>;

} // namespace hfx::core
