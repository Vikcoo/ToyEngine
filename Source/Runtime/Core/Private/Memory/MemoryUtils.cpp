// ToyEngine Core Module
// 内存工具函数实现

#include "Memory/MemoryUtils.h"

#include "Log/Log.h"

namespace TE {

const char* MemoryTagName(MemoryTag tag)
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

FormattedBytes FormatBytes(std::uint64_t bytes)
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

void DumpMemoryStats()
{
    MemoryStats stats = GetMemoryStats();

    auto total = FormatBytes(stats.CurrentBytes);
    auto peak = FormatBytes(stats.PeakBytes);

    TE_LOG_INFO("=== Memory Stats ===");
    TE_LOG_INFO("  Total:  {:.2f} {} (peak {:.2f} {})",
        total.Value, total.Unit, peak.Value, peak.Unit);
    TE_LOG_INFO("  Allocs: {}  Frees: {}  Live: {}",
        stats.AllocCount, stats.FreeCount,
        stats.AllocCount - stats.FreeCount);

    for (std::size_t i = 0; i < static_cast<std::size_t>(MemoryTag::Count); ++i)
    {
        const auto& tagStats = stats.PerTag[i];
        if (tagStats.AllocCount == 0 && tagStats.CurrentBytes == 0)
        {
            continue; // 跳过未使用的 tag
        }

        auto current = FormatBytes(tagStats.CurrentBytes);
        auto peakTag = FormatBytes(tagStats.PeakBytes);
        const char* name = MemoryTagName(static_cast<MemoryTag>(i));

        TE_LOG_INFO("  [{:<10}] {:.2f} {} (peak {:.2f} {})  allocs: {}  frees: {}",
            name, current.Value, current.Unit, peakTag.Value, peakTag.Unit,
            tagStats.AllocCount, tagStats.FreeCount);
    }

    TE_LOG_INFO("====================");
}

} // namespace TE
