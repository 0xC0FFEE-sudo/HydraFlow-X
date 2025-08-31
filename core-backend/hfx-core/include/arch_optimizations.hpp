/**
 * @file arch_optimizations.hpp
 * @brief Architecture-specific optimizations for HFT performance
 * @version 1.0.0
 * 
 * Platform-specific optimizations for Apple Silicon (ARM64) and Intel (x86_64) Macs.
 * Provides inline assembly, SIMD intrinsics, and hardware-specific features.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <chrono>

#ifdef __APPLE__
#include <mach/mach_time.h>
#include <sys/sysctl.h>
#endif

// Architecture detection
#if defined(__arm64__) || defined(__aarch64__)
    #define HFX_ARCH_ARM64 1
    #define HFX_ARCH_X86_64 0
    #include <arm_neon.h>
#elif defined(__x86_64__) || defined(_M_X64)
    #define HFX_ARCH_ARM64 0
    #define HFX_ARCH_X86_64 1
    #include <immintrin.h>
    #include <x86intrin.h>
#else
    #define HFX_ARCH_ARM64 0
    #define HFX_ARCH_X86_64 0
    #warning "Unknown architecture - optimizations disabled"
#endif

namespace hfx::core::arch {

/**
 * @brief High-resolution timestamp functions
 */
namespace timing {

#if HFX_ARCH_ARM64
    /**
     * @brief Get high-resolution timestamp (Apple Silicon)
     * Uses mach_absolute_time() with timebase conversion for nanosecond precision
     */
    inline std::uint64_t get_timestamp_ns() noexcept {
        static mach_timebase_info_data_t timebase_info = {0, 0};
        if (__builtin_expect(timebase_info.denom == 0, 0)) {
            mach_timebase_info(&timebase_info);
        }
        return mach_absolute_time() * timebase_info.numer / timebase_info.denom;
    }
    
    /**
     * @brief Get CPU cycle counter (ARM64)
     * Uses CNTVCT_EL0 virtual timer counter
     */
    inline std::uint64_t get_cycles() noexcept {
        std::uint64_t val;
        asm volatile("mrs %0, cntvct_el0" : "=r"(val));
        return val;
    }
    
    /**
     * @brief CPU frequency for cycle-to-time conversion
     */
    inline std::uint64_t get_cpu_frequency() noexcept {
        static std::uint64_t freq = 0;
        if (__builtin_expect(freq == 0, 0)) {
            std::uint64_t val;
            asm volatile("mrs %0, cntfrq_el0" : "=r"(val));
            freq = val;
        }
        return freq;
    }

#elif HFX_ARCH_X86_64
    /**
     * @brief Get high-resolution timestamp (Intel)
     * Uses RDTSC for maximum precision
     */
    inline std::uint64_t get_timestamp_ns() noexcept {
#ifdef __APPLE__
        static mach_timebase_info_data_t timebase_info = {0, 0};
        if (__builtin_expect(timebase_info.denom == 0, 0)) {
            mach_timebase_info(&timebase_info);
        }
        return mach_absolute_time() * timebase_info.numer / timebase_info.denom;
#else
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
    }
    
    /**
     * @brief Get CPU cycle counter (x86_64)
     * Uses RDTSC instruction for cycle counting
     */
    inline std::uint64_t get_cycles() noexcept {
        return __rdtsc();
    }
    
    /**
     * @brief Get CPU frequency for cycle-to-time conversion
     */
    inline std::uint64_t get_cpu_frequency() noexcept {
        static std::uint64_t freq = 0;
        if (__builtin_expect(freq == 0, 0)) {
            // Estimate frequency using timebase
#ifdef __APPLE__
            size_t size = sizeof(freq);
            sysctlbyname("hw.cpufrequency_max", &freq, &size, nullptr, 0);
            if (freq == 0) {
                freq = 3000000000ULL; // Default to 3GHz estimate
            }
#else
            freq = 3000000000ULL; // Default to 3GHz
#endif
        }
        return freq;
    }

#else
    // Fallback implementations
    inline std::uint64_t get_timestamp_ns() noexcept {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
    
    inline std::uint64_t get_cycles() noexcept {
        return get_timestamp_ns();
    }
    
    inline std::uint64_t get_cpu_frequency() noexcept {
        return 1000000000ULL; // 1 GHz default
    }
#endif

} // namespace timing

/**
 * @brief Memory optimization utilities
 */
namespace memory {

    /**
     * @brief Prefetch data into cache
     */
    inline void prefetch_read(const void* ptr) noexcept {
#if HFX_ARCH_ARM64
        __builtin_prefetch(ptr, 0, 3); // Read, high temporal locality
#elif HFX_ARCH_X86_64
        _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T0);
#else
        __builtin_prefetch(ptr, 0, 3);
#endif
    }
    
    /**
     * @brief Prefetch data for writing
     */
    inline void prefetch_write(const void* ptr) noexcept {
#if HFX_ARCH_ARM64
        __builtin_prefetch(ptr, 1, 3); // Write, high temporal locality
#elif HFX_ARCH_X86_64
        _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T0);
#else
        __builtin_prefetch(ptr, 1, 3);
#endif
    }
    
    /**
     * @brief Memory fence for ordering
     */
    inline void memory_fence() noexcept {
#if HFX_ARCH_ARM64
        asm volatile("dmb sy" ::: "memory");
#elif HFX_ARCH_X86_64
        _mm_mfence();
#else
        __sync_synchronize();
#endif
    }
    
    /**
     * @brief Cache line size detection
     */
    inline std::size_t get_cache_line_size() noexcept {
        static std::size_t cache_line_size = 0;
        if (__builtin_expect(cache_line_size == 0, 0)) {
#ifdef __APPLE__
            size_t size = sizeof(cache_line_size);
            sysctlbyname("hw.cachelinesize", &cache_line_size, &size, nullptr, 0);
#endif
            if (cache_line_size == 0) {
#if HFX_ARCH_ARM64
                cache_line_size = 64; // Apple Silicon cache line size
#else
                cache_line_size = 64; // Intel cache line size
#endif
            }
        }
        return cache_line_size;
    }

} // namespace memory

/**
 * @brief SIMD optimizations
 */
namespace simd {

#if HFX_ARCH_ARM64
    /**
     * @brief Fast memory copy using NEON (ARM64)
     */
    inline void fast_memcpy(void* dst, const void* src, std::size_t size) noexcept {
        if (size >= 64 && ((reinterpret_cast<uintptr_t>(dst) | reinterpret_cast<uintptr_t>(src)) & 15) == 0) {
            // Use NEON for aligned, large copies
            const uint8x16_t* src_vec = static_cast<const uint8x16_t*>(src);
            uint8x16_t* dst_vec = static_cast<uint8x16_t*>(dst);
            std::size_t vec_count = size / 16;
            
            for (std::size_t i = 0; i < vec_count; ++i) {
                vst1q_u8(reinterpret_cast<uint8_t*>(&dst_vec[i]), vld1q_u8(reinterpret_cast<const uint8_t*>(&src_vec[i])));
            }
            
            // Handle remaining bytes
            std::size_t remaining = size % 16;
            if (remaining > 0) {
                std::memcpy(static_cast<char*>(dst) + size - remaining,
                           static_cast<const char*>(src) + size - remaining, remaining);
            }
        } else {
            std::memcpy(dst, src, size);
        }
    }
    
    /**
     * @brief Vectorized checksum calculation (ARM64)
     */
    inline std::uint32_t fast_checksum(const void* data, std::size_t size) noexcept {
        const uint8_t* ptr = static_cast<const uint8_t*>(data);
        uint32x4_t sum = vdupq_n_u32(0);
        
        std::size_t vec_count = size / 16;
        for (std::size_t i = 0; i < vec_count; ++i) {
            uint8x16_t chunk = vld1q_u8(ptr + i * 16);
            uint16x8_t extended = vmovl_u8(vget_low_u8(chunk));
            sum = vaddq_u32(sum, vmovl_u16(vget_low_u16(extended)));
        }
        
        // Horizontal sum
        uint32_t result = vaddvq_u32(sum);
        
        // Handle remaining bytes
        for (std::size_t i = vec_count * 16; i < size; ++i) {
            result += ptr[i];
        }
        
        return result;
    }

#elif HFX_ARCH_X86_64
    /**
     * @brief Fast memory copy using AVX2 (x86_64)
     */
    inline void fast_memcpy(void* dst, const void* src, std::size_t size) noexcept {
        if (size >= 128 && ((reinterpret_cast<uintptr_t>(dst) | reinterpret_cast<uintptr_t>(src)) & 31) == 0) {
            // Use AVX2 for aligned, large copies
            const __m256i* src_vec = static_cast<const __m256i*>(src);
            __m256i* dst_vec = static_cast<__m256i*>(dst);
            std::size_t vec_count = size / 32;
            
            for (std::size_t i = 0; i < vec_count; ++i) {
                _mm256_store_si256(&dst_vec[i], _mm256_load_si256(&src_vec[i]));
            }
            
            // Handle remaining bytes
            std::size_t remaining = size % 32;
            if (remaining > 0) {
                std::memcpy(static_cast<char*>(dst) + size - remaining,
                           static_cast<const char*>(src) + size - remaining, remaining);
            }
        } else {
            std::memcpy(dst, src, size);
        }
    }
    
    /**
     * @brief Vectorized checksum calculation (x86_64)
     */
    inline std::uint32_t fast_checksum(const void* data, std::size_t size) noexcept {
        const uint8_t* ptr = static_cast<const uint8_t*>(data);
        __m256i sum = _mm256_setzero_si256();
        
        std::size_t vec_count = size / 32;
        for (std::size_t i = 0; i < vec_count; ++i) {
            __m256i chunk = _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr + i * 32));
            sum = _mm256_add_epi32(sum, _mm256_sad_epu8(chunk, _mm256_setzero_si256()));
        }
        
        // Horizontal sum
        __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
        sum128 = _mm_add_epi32(sum128, _mm_srli_si128(sum128, 8));
        sum128 = _mm_add_epi32(sum128, _mm_srli_si128(sum128, 4));
        std::uint32_t result = _mm_cvtsi128_si32(sum128);
        
        // Handle remaining bytes
        for (std::size_t i = vec_count * 32; i < size; ++i) {
            result += ptr[i];
        }
        
        return result;
    }

#else
    // Fallback implementations
    inline void fast_memcpy(void* dst, const void* src, std::size_t size) noexcept {
        std::memcpy(dst, src, size);
    }
    
    inline std::uint32_t fast_checksum(const void* data, std::size_t size) noexcept {
        const uint8_t* ptr = static_cast<const uint8_t*>(data);
        std::uint32_t sum = 0;
        for (std::size_t i = 0; i < size; ++i) {
            sum += ptr[i];
        }
        return sum;
    }
#endif

} // namespace simd

/**
 * @brief CPU optimization utilities
 */
namespace cpu {

    /**
     * @brief Pause/yield instruction for spin loops
     */
    inline void pause() noexcept {
#if HFX_ARCH_ARM64
        asm volatile("yield" ::: "memory");
#elif HFX_ARCH_X86_64
        _mm_pause();
#else
        std::this_thread::yield();
#endif
    }
    
    /**
     * @brief Get CPU core count
     */
    inline std::uint32_t get_core_count() noexcept {
        static std::uint32_t core_count = 0;
        if (__builtin_expect(core_count == 0, 0)) {
#ifdef __APPLE__
            size_t size = sizeof(core_count);
            sysctlbyname("hw.physicalcpu", &core_count, &size, nullptr, 0);
#else
            core_count = std::thread::hardware_concurrency();
#endif
            if (core_count == 0) {
                core_count = 1;
            }
        }
        return core_count;
    }
    
    /**
     * @brief Set thread affinity to specific core
     */
    inline bool set_thread_affinity(std::uint32_t core_id) noexcept {
#ifdef __APPLE__
        // macOS doesn't have direct thread affinity, but we can use thread policies
        thread_affinity_policy_data_t policy = { static_cast<integer_t>(core_id) };
        return thread_policy_set(mach_thread_self(), THREAD_AFFINITY_POLICY,
                                (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT) == KERN_SUCCESS;
#else
        // Linux implementation would go here
        (void)core_id;
        return false;
#endif
    }
    
    /**
     * @brief Set thread priority for real-time scheduling
     */
    inline bool set_realtime_priority() noexcept {
#ifdef __APPLE__
        thread_time_constraint_policy_data_t policy;
        policy.period = 0;
        policy.computation = 1000000; // 1ms
        policy.constraint = 2000000;  // 2ms
        policy.preemptible = FALSE;
        
        return thread_policy_set(mach_thread_self(), THREAD_TIME_CONSTRAINT_POLICY,
                                (thread_policy_t)&policy, THREAD_TIME_CONSTRAINT_POLICY_COUNT) == KERN_SUCCESS;
#else
        // Linux implementation would go here
        return false;
#endif
    }

} // namespace cpu

/**
 * @brief Performance monitoring utilities
 */
namespace perf {
    
    /**
     * @brief CPU performance counter structure
     */
    struct PerfCounters {
        std::uint64_t cycles;
        std::uint64_t instructions;
        std::uint64_t cache_misses;
        std::uint64_t branch_misses;
        std::uint64_t timestamp_ns;
    };
    
    /**
     * @brief Get performance counters (architecture-specific)
     */
    inline PerfCounters get_counters() noexcept {
        PerfCounters counters;
        counters.timestamp_ns = timing::get_timestamp_ns();
        counters.cycles = timing::get_cycles();
        
        // Note: Instructions and cache counters require privileged access on macOS
        // In production, these would be implemented via Instruments or dtrace
        counters.instructions = 0;
        counters.cache_misses = 0;
        counters.branch_misses = 0;
        
        return counters;
    }

} // namespace perf

} // namespace hfx::core::arch
