// ToyEngine Core Module
// 引擎级内存分配入口（默认实现：TLSF）

#pragma once

#include "MemoryTag.h"

#include <cstddef>
#include <cstdint>
#include <array>

namespace TE {

struct MemoryTagStats
{
    std::uint64_t CurrentBytes = 0;
    std::uint64_t PeakBytes = 0;
    std::uint64_t AllocCount = 0;
    std::uint64_t FreeCount = 0;
};

struct MemoryStats
{
    std::uint64_t CurrentBytes = 0;
    std::uint64_t PeakBytes = 0;
    std::uint64_t AllocCount = 0;
    std::uint64_t FreeCount = 0;

    std::array<MemoryTagStats, static_cast<std::size_t>(MemoryTag::Count)> PerTag{};
};

// 初始化默认内存系统（建议在引擎启动早期调用）
void MemoryInit(std::size_t initialBytes = 256ull * 1024ull * 1024ull);
void MemoryShutdown();

// 全局分配接口（返回值必须保存或交给 MemFree，否则泄漏）
[[nodiscard]] void* MemAlloc(std::size_t size, MemoryTag tag = MemoryTag::Unknown);
[[nodiscard]] void* MemAlignedAlloc(std::size_t size, std::size_t align, MemoryTag tag = MemoryTag::Unknown);
[[nodiscard]] void* MemAlignedRealloc(void* ptr, std::size_t newSize, std::size_t align, MemoryTag tag = MemoryTag::Unknown);
[[nodiscard]] void* MemRealloc(void* ptr, std::size_t newSize, MemoryTag tag = MemoryTag::Unknown);
void  MemFree(void* ptr);

[[nodiscard]] MemoryStats GetMemoryStats();

} // namespace TE

