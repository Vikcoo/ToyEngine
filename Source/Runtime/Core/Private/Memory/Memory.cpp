// ToyEngine Core Module
// 内存系统全局入口实现

#include "Memory/Memory.h"

#include "Memory/TlsfAllocator.h"

#include <mutex>
#include <memory>

namespace TE {

namespace {

std::mutex g_memoryMutex;
std::unique_ptr<TlsfAllocator> g_allocator;

TlsfAllocator* GetAllocator()
{
    std::scoped_lock lock(g_memoryMutex);
    return g_allocator.get();
}

} // namespace

void MemoryInit(std::size_t initialBytes)
{
    std::scoped_lock lock(g_memoryMutex);
    if (g_allocator)
    {
        return;
    }

    g_allocator = std::make_unique<TlsfAllocator>(initialBytes);
}

void MemoryShutdown()
{
    std::scoped_lock lock(g_memoryMutex);
    g_allocator.reset();
}

void* MemAlloc(std::size_t size, MemoryTag tag)
{
    return MemAlignedAlloc(size, 0, tag);
}

void* MemAlignedAlloc(std::size_t size, std::size_t align, MemoryTag tag)
{
    auto* alloc = GetAllocator();
    if (!alloc)
    {
        // 默认懒初始化，避免遗漏初始化导致崩溃
        MemoryInit();
        alloc = GetAllocator();
        if (!alloc)
        {
            return nullptr;
        }
    }
    return alloc->Allocate(size, align, tag);
}

void* MemRealloc(void* ptr, std::size_t newSize, MemoryTag tag)
{
    auto* alloc = GetAllocator();
    if (!alloc)
    {
        MemoryInit();
        alloc = GetAllocator();
        if (!alloc)
        {
            return nullptr;
        }
    }
    // 这里 align 传 0：保持“最小默认对齐”，如果你后续需要对齐 realloc，再加一个带 align 的 API
    return alloc->Reallocate(ptr, newSize, 0, tag);
}

void MemFree(void* ptr)
{
    auto* alloc = GetAllocator();
    if (!alloc)
    {
        return;
    }
    alloc->Free(ptr);
}

MemoryStats GetMemoryStats()
{
    auto* alloc = GetAllocator();
    if (!alloc)
    {
        return {};
    }
    return alloc->GetStats();
}

} // namespace TE

