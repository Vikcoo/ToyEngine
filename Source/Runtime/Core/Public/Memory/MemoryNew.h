// ToyEngine Core Module
// 引擎对象分配工具 — 显式走 TLSF，替代裸 new/delete
//
// 用法：
//   auto* actor = TE::New<TActor>(MemoryTag::Core, "Enemy");
//   TE::Delete(actor);
//
//   auto ptr = TE::MakeUnique<TActor>(MemoryTag::Core, "Enemy");
//   TUniquePtr<TActor> ptr2 = std::move(ptr);

#pragma once

#include "Memory.h"

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace TE {

// ============================================================
// TE::New<T> — placement new 到 TLSF 分配的内存
// ============================================================

/// 在 TLSF 上分配并构造一个 T 对象。
/// @tparam T      要构造的类型
/// @param  tag    内存分类标签（用于统计）
/// @param  args   转发给 T 构造函数的参数
/// @return        指向新对象的指针；分配失败抛 std::bad_alloc
template<typename T, typename... Args>
[[nodiscard]] T* New(MemoryTag tag, Args&&... args)
{
    void* mem = MemAlignedAlloc(sizeof(T), alignof(T), tag);
    if (!mem)
    {
        throw std::bad_alloc{};
    }
    return ::new (mem) T(std::forward<Args>(args)...);
}

// ============================================================
// TE::Delete<T> — 析构 + MemFree
// ============================================================

/// 析构并释放由 TE::New 分配的对象。
template<typename T>
void Delete(T* ptr)
{
    if (ptr)
    {
        ptr->~T();
        MemFree(ptr);
    }
}

// ============================================================
// TE::NewArray / TE::DeleteArray — 数组版本
// ============================================================

/// 在 TLSF 上分配 count 个 T，默认构造。
/// @return 指向首元素的指针；分配失败抛 std::bad_alloc
template<typename T>
[[nodiscard]] T* NewArray(std::size_t count, MemoryTag tag)
{
    static_assert(std::is_default_constructible_v<T>,
                  "TE::NewArray requires T to be default-constructible");
    if (count == 0)
    {
        return nullptr;
    }

    void* mem = MemAlignedAlloc(sizeof(T) * count, alignof(T), tag);
    if (!mem)
    {
        throw std::bad_alloc{};
    }

    T* arr = static_cast<T*>(mem);
    if constexpr (!std::is_trivially_constructible_v<T>)
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            ::new (static_cast<void*>(arr + i)) T();
        }
    }
    return arr;
}

/// 析构并释放由 TE::NewArray 分配的数组。
template<typename T>
void DeleteArray(T* ptr, std::size_t count)
{
    if (ptr)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (std::size_t i = count; i > 0; --i)
            {
                ptr[i - 1].~T();
            }
        }
        MemFree(ptr);
    }
}

// ============================================================
// TMemDeleter — 自定义 deleter，配合 unique_ptr 使用
// ============================================================

struct TMemDeleter
{
    template<typename T>
    void operator()(T* ptr) const
    {
        Delete(ptr);
    }
};

// ============================================================
// TUniquePtr<T> — 引擎版 unique_ptr（走 TLSF 释放）
// ============================================================

template<typename T>
using TUniquePtr = std::unique_ptr<T, TMemDeleter>;

/// 构造一个 TUniquePtr<T>，在 TLSF 上分配。
template<typename T, typename... Args>
[[nodiscard]] TUniquePtr<T> MakeUnique(MemoryTag tag, Args&&... args)
{
    return TUniquePtr<T>(New<T>(tag, std::forward<Args>(args)...));
}

// ============================================================
// TSharedPtr<T> — 引擎版 shared_ptr（对象本身走 TLSF）
// ============================================================

/// 注意：shared_ptr 的控制块（引用计数）仍走系统 malloc，
/// 因为 std::allocate_shared 的 Allocator 要求比较复杂。
/// 只有被管理对象本身走 TLSF 分配。
///
/// 如果将来需要控制块也走 TLSF，可以改用 allocate_shared + TEngineAllocator。
template<typename T>
using TSharedPtr = std::shared_ptr<T>;

/// 构造一个 TSharedPtr<T>，对象在 TLSF 上分配。
template<typename T, typename... Args>
[[nodiscard]] TSharedPtr<T> MakeShared(MemoryTag tag, Args&&... args)
{
    return TSharedPtr<T>(New<T>(tag, std::forward<Args>(args)...),
                         [](T* ptr) { Delete(ptr); });
}

template<typename T>
using TWeakPtr = std::weak_ptr<T>;

} // namespace TE
