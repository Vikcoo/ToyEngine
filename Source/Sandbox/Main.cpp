// ToyEngine Sandbox - 完整渲染流程测试
#include "Log/Log.h"
#include "Memory/Memory.h"
#include "Window.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

int main()
{
    // 初始化日志系统
    TE::Log::Init();

    // 初始化内存系统：故意给小一点以测试扩容
    TE::MemoryInit(32ull * 1024ull * 1024ull);

    // 简单自检：不同 tag / 不同对齐 / 触发扩容
    void* a = TE::MemAlloc(1024, TE::MemoryTag::Core);
    void* b = TE::MemAlignedAlloc(4096, 64, TE::MemoryTag::Renderer);
    void* c = TE::MemAlloc(20ull * 1024ull * 1024ull, TE::MemoryTag::Asset);
    c = TE::MemRealloc(c, 48ull * 1024ull * 1024ull, TE::MemoryTag::Asset); // 大概率触发 add_pool

    const auto stats = TE::GetMemoryStats();
    TE_LOG_INFO("Memory: current={} bytes, peak={} bytes, alloc={}, free={}",
        stats.CurrentBytes, stats.PeakBytes, stats.AllocCount, stats.FreeCount);
    TE_LOG_INFO("Memory(Core): current={} peak={} alloc={} free={}",
        stats.PerTag[static_cast<std::size_t>(TE::MemoryTag::Core)].CurrentBytes,
        stats.PerTag[static_cast<std::size_t>(TE::MemoryTag::Core)].PeakBytes,
        stats.PerTag[static_cast<std::size_t>(TE::MemoryTag::Core)].AllocCount,
        stats.PerTag[static_cast<std::size_t>(TE::MemoryTag::Core)].FreeCount);
    TE_LOG_INFO("Memory(Renderer): current={} peak={} alloc={} free={}",
        stats.PerTag[static_cast<std::size_t>(TE::MemoryTag::Renderer)].CurrentBytes,
        stats.PerTag[static_cast<std::size_t>(TE::MemoryTag::Renderer)].PeakBytes,
        stats.PerTag[static_cast<std::size_t>(TE::MemoryTag::Renderer)].AllocCount,
        stats.PerTag[static_cast<std::size_t>(TE::MemoryTag::Renderer)].FreeCount);
    TE_LOG_INFO("Memory(Asset): current={} peak={} alloc={} free={}",
        stats.PerTag[static_cast<std::size_t>(TE::MemoryTag::Asset)].CurrentBytes,
        stats.PerTag[static_cast<std::size_t>(TE::MemoryTag::Asset)].PeakBytes,
        stats.PerTag[static_cast<std::size_t>(TE::MemoryTag::Asset)].AllocCount,
        stats.PerTag[static_cast<std::size_t>(TE::MemoryTag::Asset)].FreeCount);

    TE::MemFree(a);
    TE::MemFree(b);
    TE::MemFree(c);

    /* 1. 创建窗口 */
    const TE::WindowConfig config{"ToyEngine", 1280, 720, true};
    const auto window = TE::Window::Create(config);

    system("pause");

    return 0;
}