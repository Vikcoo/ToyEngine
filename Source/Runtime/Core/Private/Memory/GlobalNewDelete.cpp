// ToyEngine Core Module
// 可选：覆盖全局 new/delete，让 STL/默认 new 走引擎分配器

#include "Memory/Memory.h"

#include <new>

// 通过编译宏控制，避免强制影响所有目标
// 在 CMake 中可对特定 target 添加：target_compile_definitions(xxx PRIVATE TE_ENABLE_GLOBAL_NEW_DELETE=1)
#if defined(TE_ENABLE_GLOBAL_NEW_DELETE) && TE_ENABLE_GLOBAL_NEW_DELETE

void* operator new(std::size_t size)
{
    if (void* p = TE::MemAlloc(size, TE::MemoryTag::STL))
    {
        return p;
    }
    throw std::bad_alloc{};
}

void operator delete(void* ptr) noexcept
{
    TE::MemFree(ptr);
}

void* operator new[](std::size_t size)
{
    if (void* p = TE::MemAlloc(size, TE::MemoryTag::STL))
    {
        return p;
    }
    throw std::bad_alloc{};
}

void operator delete[](void* ptr) noexcept
{
    TE::MemFree(ptr);
}

// C++14 sized delete（MSVC 可能会用到）
void operator delete(void* ptr, std::size_t /*size*/) noexcept
{
    TE::MemFree(ptr);
}

void operator delete[](void* ptr, std::size_t /*size*/) noexcept
{
    TE::MemFree(ptr);
}

#endif

