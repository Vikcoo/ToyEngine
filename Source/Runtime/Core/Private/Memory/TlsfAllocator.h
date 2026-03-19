// ToyEngine Core Module
// TLSF 分配器封装（引擎默认堆）

#pragma once

#include "Memory/Memory.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

// TLSF 是 C 实现
extern "C" {
#include "tlsf.h"
}

namespace TE {

class TlsfAllocator final
{
public:
    explicit TlsfAllocator(std::size_t initialBytes);
    ~TlsfAllocator();

    TlsfAllocator(const TlsfAllocator&) = delete;
    TlsfAllocator& operator=(const TlsfAllocator&) = delete;

    void* Allocate(std::size_t size, std::size_t align, MemoryTag tag);
    void* Reallocate(void* userPtr, std::size_t newSize, std::size_t align, MemoryTag tag);
    void  Free(void* userPtr);

    MemoryStats GetStats() const;

private:
    struct PoolRecord
    {
        void* Base = nullptr;
        std::size_t Bytes = 0;
        pool_t Pool = nullptr;
    };

    struct AllocHeader
    {
        void* RawPtr = nullptr;           // TLSF 返回的原始指针（用于 free/realloc）
        std::uint32_t Magic = 0;          // 调试用
        std::uint32_t Alignment = 0;      // 用户请求对齐（realloc 时用于保留原语义）
        MemoryTag Tag = MemoryTag::Unknown;
        std::uint64_t RequestedBytes = 0; // 用户请求大小（用于统计）
    };

    static constexpr std::uint32_t HeaderMagic = 0x54454D4D; // 'T''E''M''M'

    static std::size_t DefaultAlign();
    static std::size_t NormalizeAlign(std::size_t align);
    static bool IsPowerOfTwo(std::size_t x);
    static std::uintptr_t AlignUp(std::uintptr_t x, std::size_t align);
    static bool CheckedAdd(std::size_t a, std::size_t b, std::size_t& out);

    // OS 后备内存
    static void* OsReserveCommit(std::size_t bytes, std::size_t align);
    static void  OsRelease(void* ptr, std::size_t bytes);

    bool EnsureInitializedLocked();
    bool AddPoolLocked(std::size_t bytes);
    void* AllocateLocked(std::size_t size, std::size_t align, MemoryTag tag);
    void  FreeLocked(void* userPtr);

    static AllocHeader* HeaderFromUserPtr(void* userPtr);

    void OnAllocLocked(MemoryTag tag, std::uint64_t bytes);
    void OnFreeLocked(MemoryTag tag, std::uint64_t bytes);

private:
    const std::size_t m_initialBytes = 0;
    std::size_t m_nextGrowBytes = 0;

    mutable std::mutex m_mutex;
    tlsf_t m_tlsf = nullptr;
    std::vector<PoolRecord> m_pools;
    MemoryStats m_stats{};
};

} // namespace TE

