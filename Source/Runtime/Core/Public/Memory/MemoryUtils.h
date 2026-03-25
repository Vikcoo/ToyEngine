// ToyEngine Core Module
// 内存工具函数 — Tag 名称映射、统计信息格式化
//
// 用法：
//   const char* name = TE::MemoryTagName(TE::MemoryTag::RHI);  // → "RHI"
//   TE::DumpMemoryStats();  // 打印完整统计到日志

#pragma once

#include "Memory.h"
#include "Log/Log.h"

#include <cstddef>

namespace TE {

// ============================================================
// MemoryTagName — 获取 MemoryTag 的可读名称
// ============================================================

inline const char* MemoryTagName(MemoryTag tag)
{
    switch (tag)
    {
    case MemoryTag::Unknown:   return "Unknown";
    case MemoryTag::STL:       return "STL";
    case MemoryTag::Core:      return "Core";
    case MemoryTag::Platform:  return "Platform";
    case MemoryTag::RHI:       return "RHI";
    case MemoryTag::Renderer:  return "Renderer";
    case MemoryTag::Asset:     return "Asset";
    case MemoryTag::Scene:     return "Scene";
    case MemoryTag::Sandbox:   return "Sandbox";
    default:                   return "???";
    }
}

// ============================================================
// 字节格式化辅助
// ============================================================

struct FormattedBytes
{
    double Value;
    const char* Unit;
};

inline FormattedBytes FormatBytes(std::uint64_t bytes)
{
    if (bytes >= 1024ull * 1024ull * 1024ull)
    {
        return { static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0), "GB" };
    }
    if (bytes >= 1024ull * 1024ull)
    {
        return { static_cast<double>(bytes) / (1024.0 * 1024.0), "MB" };
    }
    if (bytes >= 1024ull)
    {
        return { static_cast<double>(bytes) / 1024.0, "KB" };
    }
    return { static_cast<double>(bytes), "B" };
}

// ============================================================
// DumpMemoryStats — 打印完整内存统计到日志
// ============================================================

inline void DumpMemoryStats()
{
    MemoryStats stats = GetMemoryStats();

    auto total = FormatBytes(stats.CurrentBytes);
    auto peak  = FormatBytes(stats.PeakBytes);

    TE_LOG_INFO("=== Memory Stats ===");
    TE_LOG_INFO("  Total:  {:.2f} {} (peak {:.2f} {})",
                total.Value, total.Unit, peak.Value, peak.Unit);
    TE_LOG_INFO("  Allocs: {}  Frees: {}  Live: {}",
                stats.AllocCount, stats.FreeCount,
                stats.AllocCount - stats.FreeCount);

    for (std::size_t i = 0; i < static_cast<std::size_t>(MemoryTag::Count); ++i)
    {
        const auto& t = stats.PerTag[i];
        if (t.AllocCount == 0 && t.CurrentBytes == 0)
        {
            continue; // 跳过未使用的 tag
        }

        auto cur  = FormatBytes(t.CurrentBytes);
        auto pk   = FormatBytes(t.PeakBytes);
        const char* name = MemoryTagName(static_cast<MemoryTag>(i));

        TE_LOG_INFO("  [{:<10}] {:.2f} {} (peak {:.2f} {})  allocs: {}  frees: {}",
                    name, cur.Value, cur.Unit, pk.Value, pk.Unit,
                    t.AllocCount, t.FreeCount);
    }

    TE_LOG_INFO("====================");
}

} // namespace TE
