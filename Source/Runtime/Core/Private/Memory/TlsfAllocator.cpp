// ToyEngine Core Module
// TLSF 分配器封装实现

#include "Memory/TlsfAllocator.h"

#include <algorithm>
#include <cstring>
#include <new>

#if defined(_WIN32)
// 避免 Windows 头文件定义 min/max 宏，破坏 std::min/std::max
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace TE {

static std::size_t ClampGrow(std::size_t bytes)
{
    constexpr std::size_t MinGrow = 16ull * 1024ull * 1024ull;
    constexpr std::size_t MaxGrow = 512ull * 1024ull * 1024ull;
    return std::clamp(bytes, MinGrow, MaxGrow);
}

TlsfAllocator::TlsfAllocator(std::size_t initialBytes)
    : m_initialBytes(initialBytes)
    , m_nextGrowBytes(ClampGrow(initialBytes / 2))
{
}

TlsfAllocator::~TlsfAllocator()
{
    std::scoped_lock lock(m_mutex);

    // TLSF destroy 不做事，但调用以保持语义完整
    if (m_tlsf)
    {
        tlsf_destroy(m_tlsf);
        m_tlsf = nullptr;
    }

    for (const auto& pool : m_pools)
    {
        if (pool.Base && pool.Bytes)
        {
            OsRelease(pool.Base, pool.Bytes);
        }
    }
    m_pools.clear();
}

std::size_t TlsfAllocator::DefaultAlign()
{
    return tlsf_align_size();
}

bool TlsfAllocator::IsPowerOfTwo(std::size_t x)
{
    return x != 0 && (x & (x - 1)) == 0;
}

std::uintptr_t TlsfAllocator::AlignUp(std::uintptr_t x, std::size_t align)
{
    return (x + (align - 1)) & ~(static_cast<std::uintptr_t>(align - 1));
}

void* TlsfAllocator::OsReserveCommit(std::size_t bytes, std::size_t align)
{
    (void)align;

#if defined(_WIN32)
    // VirtualAlloc 返回的地址通常满足页对齐（>= 4KB），足以覆盖 tlsf_align_size(4/8)。
    return ::VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    // TODO: 非 Windows 平台：mmap/posix_memalign 等
    return std::aligned_alloc(align, bytes);
#endif
}

void TlsfAllocator::OsRelease(void* ptr, std::size_t bytes)
{
    (void)bytes;
#if defined(_WIN32)
    ::VirtualFree(ptr, 0, MEM_RELEASE);
#else
    std::free(ptr);
#endif
}

bool TlsfAllocator::EnsureInitializedLocked()
{
    if (m_tlsf)
    {
        return true;
    }

    const std::size_t align = DefaultAlign();
    if (!IsPowerOfTwo(align))
    {
        return false;
    }

    // 第一块：control + pool 一起放在同一块大内存里
    const std::size_t bytes = std::max<std::size_t>(m_initialBytes, 64ull * 1024ull * 1024ull);
    void* base = OsReserveCommit(bytes, align);
    if (!base)
    {
        return false;
    }

    tlsf_t tlsf = tlsf_create_with_pool(base, bytes);
    if (!tlsf)
    {
        OsRelease(base, bytes);
        return false;
    }

    m_tlsf = tlsf;
    {
        PoolRecord rec;
        rec.Base = base;
        rec.Bytes = bytes;
        rec.Pool = tlsf_get_pool(m_tlsf);
        m_pools.push_back(rec);
    }

    return true;
}

bool TlsfAllocator::AddPoolLocked(std::size_t bytes)
{
    if (!m_tlsf)
    {
        return false;
    }

    const std::size_t align = DefaultAlign();
    const std::size_t growBytes = ClampGrow(bytes);

    void* base = OsReserveCommit(growBytes, align);
    if (!base)
    {
        return false;
    }

    pool_t pool = tlsf_add_pool(m_tlsf, base, growBytes);
    if (!pool)
    {
        OsRelease(base, growBytes);
        return false;
    }

    {
        PoolRecord rec;
        rec.Base = base;
        rec.Bytes = growBytes;
        rec.Pool = pool;
        m_pools.push_back(rec);
    }

    // 下一次按几何增长
    m_nextGrowBytes = ClampGrow(m_nextGrowBytes * 2);
    return true;
}

TlsfAllocator::AllocHeader* TlsfAllocator::HeaderFromUserPtr(void* userPtr)
{
    if (!userPtr)
    {
        return nullptr;
    }
    auto* header = reinterpret_cast<AllocHeader*>(
        static_cast<std::byte*>(userPtr) - sizeof(AllocHeader));
    return header;
}

void TlsfAllocator::OnAllocLocked(MemoryTag tag, std::uint64_t bytes)
{
    const auto idx = static_cast<std::size_t>(tag);
    if (idx < m_stats.PerTag.size())
    {
        auto& t = m_stats.PerTag[idx];
        t.CurrentBytes += bytes;
        t.PeakBytes = std::max(t.PeakBytes, t.CurrentBytes);
        t.AllocCount += 1;
    }

    m_stats.CurrentBytes += bytes;
    m_stats.PeakBytes = std::max(m_stats.PeakBytes, m_stats.CurrentBytes);
    m_stats.AllocCount += 1;
}

void TlsfAllocator::OnFreeLocked(MemoryTag tag, std::uint64_t bytes)
{
    const auto idx = static_cast<std::size_t>(tag);
    if (idx < m_stats.PerTag.size())
    {
        auto& t = m_stats.PerTag[idx];
        t.CurrentBytes -= bytes;
        t.FreeCount += 1;
    }

    m_stats.CurrentBytes -= bytes;
    m_stats.FreeCount += 1;
}

void* TlsfAllocator::AllocateLocked(std::size_t size, std::size_t align, MemoryTag tag)
{
    if (!EnsureInitializedLocked())
    {
        return nullptr;
    }

    if (size == 0)
    {
        return nullptr;
    }

    if (align == 0)
    {
        align = DefaultAlign();
    }
    align = std::max(align, DefaultAlign());
    if (!IsPowerOfTwo(align))
    {
        return nullptr;
    }

    const std::size_t total = size + sizeof(AllocHeader) + (align - 1);

    void* raw = tlsf_malloc(m_tlsf, total);
    if (!raw)
    {
        // 尝试扩容一次后重试
        if (AddPoolLocked(m_nextGrowBytes))
        {
            raw = tlsf_malloc(m_tlsf, total);
        }
    }
    if (!raw)
    {
        return nullptr;
    }

    const auto rawAddr = reinterpret_cast<std::uintptr_t>(raw);
    const auto userAddr = AlignUp(rawAddr + sizeof(AllocHeader), align);
    void* userPtr = reinterpret_cast<void*>(userAddr);

    auto* header = reinterpret_cast<AllocHeader*>(userAddr - sizeof(AllocHeader));
    header->RawPtr = raw;
    header->Magic = HeaderMagic;
    header->Tag = tag;
    header->RequestedBytes = static_cast<std::uint64_t>(size);

    OnAllocLocked(tag, header->RequestedBytes);
    return userPtr;
}

void* TlsfAllocator::Allocate(std::size_t size, std::size_t align, MemoryTag tag)
{
    std::scoped_lock lock(m_mutex);
    return AllocateLocked(size, align, tag);
}

void* TlsfAllocator::Reallocate(void* userPtr, std::size_t newSize, std::size_t align, MemoryTag tag)
{
    std::scoped_lock lock(m_mutex);

    if (!EnsureInitializedLocked())
    {
        return nullptr;
    }

    if (!userPtr)
    {
        return AllocateLocked(newSize, align, tag);
    }
    if (newSize == 0)
    {
        Free(userPtr);
        return nullptr;
    }

    auto* oldHeader = HeaderFromUserPtr(userPtr);
    if (!oldHeader || oldHeader->Magic != HeaderMagic)
    {
        // 指针不是来自本分配器：直接失败（保持行为明确）
        return nullptr;
    }

    // 简化策略：保持语义稳定（尤其是对齐变化时），直接 alloc + memcpy + free
    const std::uint64_t oldSize = oldHeader->RequestedBytes;
    const MemoryTag oldTag = oldHeader->Tag;

    void* newPtr = AllocateLocked(newSize, align, tag);
    if (!newPtr)
    {
        return nullptr;
    }

    std::memcpy(newPtr, userPtr, static_cast<std::size_t>(std::min<std::uint64_t>(oldSize, newSize)));
    Free(userPtr);

    // 统计：Free 里已经按 oldTag 扣除；AllocateLocked 已按 tag 增加
    (void)oldTag;
    return newPtr;
}

void TlsfAllocator::Free(void* userPtr)
{
    if (!userPtr)
    {
        return;
    }

    std::scoped_lock lock(m_mutex);

    auto* header = HeaderFromUserPtr(userPtr);
    if (!header || header->Magic != HeaderMagic)
    {
        return;
    }

    void* raw = header->RawPtr;
    const auto tag = header->Tag;
    const auto bytes = header->RequestedBytes;

    header->Magic = 0;
    header->RawPtr = nullptr;
    header->RequestedBytes = 0;

    OnFreeLocked(tag, bytes);
    tlsf_free(m_tlsf, raw);
}

MemoryStats TlsfAllocator::GetStats() const
{
    std::scoped_lock lock(m_mutex);
    return m_stats;
}

} // namespace TE

