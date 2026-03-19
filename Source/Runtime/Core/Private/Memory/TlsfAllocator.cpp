// ToyEngine Core Module
// TLSF 分配器封装实现

#include "Memory/TlsfAllocator.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <limits>
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

std::size_t TlsfAllocator::NormalizeAlign(std::size_t align)
{
    const std::size_t baseAlign = DefaultAlign();
    if (align == 0)
    {
        return baseAlign;
    }
    return std::max(align, baseAlign);
}

bool TlsfAllocator::IsPowerOfTwo(std::size_t x)
{
    return x != 0 && (x & (x - 1)) == 0;
}

std::uintptr_t TlsfAllocator::AlignUp(std::uintptr_t x, std::size_t align)
{
    return (x + (align - 1)) & ~(static_cast<std::uintptr_t>(align - 1));
}

bool TlsfAllocator::CheckedAdd(std::size_t a, std::size_t b, std::size_t& out)
{
    if (a > std::numeric_limits<std::size_t>::max() - b)
    {
        return false;
    }
    out = a + b;
    return true;
}

void* TlsfAllocator::OsReserveCommit(std::size_t bytes, std::size_t align)
{
#if defined(_WIN32)
    // VirtualAlloc 返回的地址通常满足页对齐（>= 4KB），足以覆盖 tlsf_align_size(4/8)。
    (void)align;
    return ::VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    if (!IsPowerOfTwo(align) || bytes == 0)
    {
        return nullptr;
    }
    void* ptr = nullptr;
    if (posix_memalign(&ptr, align, bytes) != 0)
    {
        return nullptr;
    }
    return ptr;
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
    const std::size_t growBytes = ClampGrow(bytes > 0 ? bytes : (16ull * 1024ull * 1024ull));

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
    m_nextGrowBytes = ClampGrow(growBytes * 2);
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
        t.CurrentBytes = (t.CurrentBytes >= bytes) ? (t.CurrentBytes - bytes) : 0;
        t.FreeCount += 1;
    }

    m_stats.CurrentBytes = (m_stats.CurrentBytes >= bytes) ? (m_stats.CurrentBytes - bytes) : 0;
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

    align = NormalizeAlign(align);
    if (!IsPowerOfTwo(align))
    {
        return nullptr;
    }
    if (align > std::numeric_limits<std::uint32_t>::max())
    {
        return nullptr;
    }

    std::size_t total = 0;
    if (!CheckedAdd(size, sizeof(AllocHeader), total))
    {
        return nullptr;
    }
    if (!CheckedAdd(total, align - 1, total))
    {
        return nullptr;
    }

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
    header->Alignment = static_cast<std::uint32_t>(align);
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
        FreeLocked(userPtr);
        return nullptr;
    }

    auto* oldHeader = HeaderFromUserPtr(userPtr);
    if (!oldHeader || oldHeader->Magic != HeaderMagic)
    {
        // 指针不是来自本分配器：直接失败（保持行为明确）
        return nullptr;
    }

    const std::uint64_t oldSize = oldHeader->RequestedBytes;
    const std::size_t oldAlign = oldHeader->Alignment != 0
        ? static_cast<std::size_t>(oldHeader->Alignment)
        : DefaultAlign();
    const MemoryTag oldTag = oldHeader->Tag;

    // 若未显式传入新对齐，则沿用旧对齐语义。
    const std::size_t effectiveAlign = (align == 0) ? oldAlign : align;
    // 若未显式传入新 tag，则沿用旧 tag。
    const MemoryTag effectiveTag = (tag == MemoryTag::Unknown) ? oldTag : tag;

    if (newSize == oldSize && effectiveAlign == oldAlign && effectiveTag == oldTag)
    {
        return userPtr;
    }

    // 保持语义稳定（尤其是对齐变化时），统一走 alloc + memcpy + free。
    void* newPtr = AllocateLocked(newSize, effectiveAlign, effectiveTag);
    if (!newPtr)
    {
        return nullptr;
    }

    std::memcpy(newPtr, userPtr, static_cast<std::size_t>(std::min<std::uint64_t>(oldSize, newSize)));
    FreeLocked(userPtr);
    return newPtr;
}

void TlsfAllocator::Free(void* userPtr)
{
    if (!userPtr)
    {
        return;
    }

    std::scoped_lock lock(m_mutex);
    FreeLocked(userPtr);
}

void TlsfAllocator::FreeLocked(void* userPtr)
{
    auto* header = HeaderFromUserPtr(userPtr);
    if (!header || header->Magic != HeaderMagic)
    {
        assert(false && "invalid pointer passed to TlsfAllocator::FreeLocked");
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

