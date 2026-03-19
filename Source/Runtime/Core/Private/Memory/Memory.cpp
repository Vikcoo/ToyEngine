// ToyEngine Core Module
// 内存系统全局入口实现

#include "Memory/Memory.h"

#include "Memory/TlsfAllocator.h"

#include <mutex>
#include <memory>

namespace TE {

namespace {

std::mutex g_memoryMutex;
std::shared_ptr<TlsfAllocator> g_allocator;

std::shared_ptr<TlsfAllocator> GetAllocatorSnapshot()
{
    std::scoped_lock lock(g_memoryMutex);
    return g_allocator;
}

std::shared_ptr<TlsfAllocator> GetOrCreateAllocator(std::size_t initialBytes = 256ull * 1024ull * 1024ull)
{
    std::scoped_lock lock(g_memoryMutex);
    if (!g_allocator)
    {
        g_allocator = std::make_shared<TlsfAllocator>(initialBytes);
    }
    return g_allocator;
}

} // namespace

void MemoryInit(std::size_t initialBytes)
{
    std::scoped_lock lock(g_memoryMutex);
    g_allocator = g_allocator ? g_allocator : std::make_shared<TlsfAllocator>(initialBytes);
}

void MemoryShutdown()
{
    std::shared_ptr<TlsfAllocator> old;
    std::scoped_lock lock(g_memoryMutex);
    old.swap(g_allocator);
}

void* MemAlloc(std::size_t size, MemoryTag tag)
{
    return MemAlignedAlloc(size, 0, tag);
}

void* MemAlignedAlloc(std::size_t size, std::size_t align, MemoryTag tag)
{
    auto alloc = GetOrCreateAllocator();
    if (!alloc)
    {
        return nullptr;
    }
    return alloc->Allocate(size, align, tag);
}

void* MemAlignedRealloc(void* ptr, std::size_t newSize, std::size_t align, MemoryTag tag)
{
    auto alloc = GetOrCreateAllocator();
    if (!alloc)
    {
        return nullptr;
    }
    return alloc->Reallocate(ptr, newSize, align, tag);
}

void* MemRealloc(void* ptr, std::size_t newSize, MemoryTag tag)
{
    return MemAlignedRealloc(ptr, newSize, 0, tag);
}

void MemFree(void* ptr)
{
    auto alloc = GetAllocatorSnapshot();
    if (!alloc)
    {
        return;
    }
    alloc->Free(ptr);
}

MemoryStats GetMemoryStats()
{
    auto alloc = GetAllocatorSnapshot();
    if (!alloc)
    {
        return {};
    }
    return alloc->GetStats();
}

} // namespace TE

