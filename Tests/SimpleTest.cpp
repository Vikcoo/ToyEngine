// ToyEngine - 内存分配器最小回归测试（多线程 / 对齐 realloc / Shutdown 竞态）
#include "Memory/Memory.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#include "Memory/MemoryUtils.h"

namespace {

bool IsAligned(void* p, std::size_t align)
{
    if (!p || align == 0)
    {
        return false;
    }
    const auto addr = reinterpret_cast<std::uintptr_t>(p);
    return (addr % align) == 0;
}

bool TestMultiThreadAllocFree()
{
    constexpr int kThreads = 8;
    constexpr int kOuter = 50;
    constexpr int kInner = 200;

    TE::MemoryInit(32ull * 1024ull * 1024ull);

    std::atomic<bool> ok{true};

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t)
    {
        threads.emplace_back([t, &ok]() {
            for (int o = 0; o < kOuter && ok.load(std::memory_order_acquire); ++o)
            {
                std::vector<void*> ptrs;
                ptrs.reserve(static_cast<std::size_t>(kInner));
                for (int i = 0; i < kInner; ++i)
                {
                    const std::size_t size = 16u + static_cast<std::size_t>((t + o + i) % 512);
                    void* p = TE::MemAlloc(size, TE::MemoryTag::Core);
                    if (!p)
                    {
                        std::cerr << "[FAIL] MemAlloc returned nullptr (thread " << t << ")\n";
                        ok.store(false, std::memory_order_release);
                        return;
                    }
                    std::memset(p, static_cast<int>(i & 0xFF), size);
                    ptrs.push_back(p);
                }
                for (void* p : ptrs)
                {
                    TE::MemFree(p);
                }
            }
        });
    }

    for (auto& th : threads)
    {
        th.join();
    }

    TE::MemoryShutdown();
    return ok.load(std::memory_order_acquire);
}

bool TestAlignedReallocPreservesAlignment()
{
    TE::MemoryInit(16ull * 1024ull * 1024ull);

    constexpr std::size_t kAlign = 64;
    constexpr std::size_t kOldSize = 256;
    constexpr std::size_t kNewSize = 512;

    void* p = TE::MemAlignedAlloc(kOldSize, kAlign, TE::MemoryTag::Renderer);
    if (!p || !IsAligned(p, kAlign))
    {
        std::cerr << "[FAIL] MemAlignedAlloc alignment\n";
        TE::MemoryShutdown();
        return false;
    }

    auto* bytes = static_cast<std::byte*>(p);
    for (std::size_t i = 0; i < kOldSize; ++i)
    {
        bytes[i] = static_cast<std::byte>(static_cast<unsigned char>(0xA5 ^ (i & 0xFF)));
    }

    // MemRealloc(align=0) 应保留原块对齐（64）
    void* q = TE::MemRealloc(p, kNewSize, TE::MemoryTag::Renderer);
    if (!q || !IsAligned(q, kAlign))
    {
        std::cerr << "[FAIL] MemRealloc lost alignment\n";
        if (q)
        {
            TE::MemFree(q);
        }
        else
        {
            TE::MemFree(p);
        }
        TE::MemoryShutdown();
        return false;
    }

    TE::DumpMemoryStats();

    auto* qbytes = static_cast<std::byte*>(q);
    for (std::size_t i = 0; i < kOldSize; ++i)
    {
        const auto expected = static_cast<std::byte>(static_cast<unsigned char>(0xA5 ^ (i & 0xFF)));
        if (qbytes[i] != expected)
        {
            std::cerr << "[FAIL] MemRealloc data corrupt at " << i << "\n";
            TE::MemFree(q);
            TE::MemoryShutdown();
            return false;
        }
    }

    TE::MemFree(q);

    // MemAlignedRealloc(..., align=0) 同样应保留对齐
    p = TE::MemAlignedAlloc(128, 32, TE::MemoryTag::Asset);
    if (!p || !IsAligned(p, 32u))
    {
        std::cerr << "[FAIL] second MemAlignedAlloc\n";
        TE::MemoryShutdown();
        return false;
    }
    std::memset(p, 0x3C, 128);
    q = TE::MemAlignedRealloc(p, 256, 0, TE::MemoryTag::Asset);
    if (!q || !IsAligned(q, 32u))
    {
        std::cerr << "[FAIL] MemAlignedRealloc(align=0) lost alignment\n";
        if (q)
        {
            TE::MemFree(q);
        }
        else
        {
            TE::MemFree(p);
        }
        TE::MemoryShutdown();
        return false;
    }
    if (static_cast<unsigned char*>(q)[0] != 0x3C)
    {
        std::cerr << "[FAIL] MemAlignedRealloc data\n";
        TE::MemFree(q);
        TE::MemoryShutdown();
        return false;
    }
    TE::MemFree(q);

    TE::MemoryShutdown();
    return true;
}

bool TestShutdownRaceNoCrash()
{
    TE::MemoryInit(24ull * 1024ull * 1024ull);

    std::atomic<bool> stop{false};
    constexpr int kWorkers = 6;

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(kWorkers));

    for (int w = 0; w < kWorkers; ++w)
    {
        workers.emplace_back([&stop, w]() {
            std::uint64_t local = 0;
            while (!stop.load(std::memory_order_acquire))
            {
                void* p = TE::MemAlloc(32u + static_cast<std::size_t>(local % 128u),
                    TE::MemoryTag::Sandbox);
                if (p)
                {
                    *static_cast<std::uint64_t*>(p) = local;
                    TE::MemFree(p);
                }
                ++local;
                if ((local & 0x3Fu) == 0u)
                {
                    std::this_thread::yield();
                }
                (void)w;
            }
        });
    }

    std::thread shutdownThread([&stop]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        TE::MemoryShutdown();
        stop.store(true, std::memory_order_release);
    });

    shutdownThread.join();
    for (auto& th : workers)
    {
        th.join();
    }

    // 进程内可能已被其它线程通过 MemAlloc 再次懒创建分配器；再关一次保证干净
    TE::MemoryShutdown();
    return true;
}

} // namespace

int main()
{
    TE::Log::Init();

    std::cout << "[MemoryAllocatorRegressionTest] multi-thread alloc/free...\n";
    if (!TestMultiThreadAllocFree())
    {
        return 1;
    }

    std::cout << "[MemoryAllocatorRegressionTest] aligned realloc...\n";
    if (!TestAlignedReallocPreservesAlignment())
    {
        return 1;
    }

    std::cout << "[MemoryAllocatorRegressionTest] shutdown race...\n";
    if (!TestShutdownRaceNoCrash())
    {
        return 1;
    }

    std::cout << "[MemoryAllocatorRegressionTest] all passed.\n";
    return 0;
}
