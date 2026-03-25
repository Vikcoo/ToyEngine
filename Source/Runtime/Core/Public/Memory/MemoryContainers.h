// ToyEngine Core Module
// STL 容器的引擎 Allocator 适配 — 让 vector/string/map 等内部分配走 TLSF
//
// 用法：
//   TE::TArray<int> positions;                 // 默认走 MemoryTag::Core
//   TE::TArray<int> rhiBuffers(TE::TEngineAllocator<int>(MemoryTag::RHI));
//   TE::TString name = "Hello";

#pragma once

#include "Memory.h"

#include <deque>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace TE {

// ============================================================
// TEngineAllocator<T> — 符合 C++ Allocator 要求的 TLSF 适配器
// ============================================================

template<typename T>
class TEngineAllocator
{
public:
    using value_type = T;

    /// 默认构造 — 使用 Core tag
    TEngineAllocator() noexcept
        : m_Tag(MemoryTag::Core)
    {
    }

    /// 指定 tag 构造
    explicit TEngineAllocator(MemoryTag tag) noexcept
        : m_Tag(tag)
    {
    }

    /// 拷贝转换构造（支持 rebind）
    template<typename U>
    TEngineAllocator(const TEngineAllocator<U>& other) noexcept
        : m_Tag(other.GetTag())
    {
    }

    MemoryTag GetTag() const noexcept { return m_Tag; }

    T* allocate(std::size_t n)
    {
        if (n == 0)
        {
            return nullptr;
        }
        void* p = MemAlignedAlloc(n * sizeof(T), alignof(T), m_Tag);
        if (!p)
        {
            throw std::bad_alloc{};
        }
        return static_cast<T*>(p);
    }

    void deallocate(T* ptr, std::size_t /*n*/) noexcept
    {
        MemFree(ptr);
    }

    /// 同 tag 的 allocator 视为相等（可互相 deallocate）
    template<typename U>
    bool operator==(const TEngineAllocator<U>& other) const noexcept
    {
        return m_Tag == other.GetTag();
    }

    template<typename U>
    bool operator!=(const TEngineAllocator<U>& other) const noexcept
    {
        return !(*this == other);
    }

private:
    MemoryTag m_Tag;
};

// ============================================================
// 引擎容器别名 — 内部分配走 TLSF
// ============================================================

/// 动态数组（替代 std::vector）
template<typename T>
using TArray = std::vector<T, TEngineAllocator<T>>;

/// 字符串（替代 std::string）
using TString = std::basic_string<char, std::char_traits<char>, TEngineAllocator<char>>;

/// 双端队列
template<typename T>
using TDeque = std::deque<T, TEngineAllocator<T>>;

/// 链表
template<typename T>
using TList = std::list<T, TEngineAllocator<T>>;

/// 有序映射
template<typename K, typename V, typename Compare = std::less<K>>
using TMap = std::map<K, V, Compare, TEngineAllocator<std::pair<const K, V>>>;

/// 有序集合
template<typename K, typename Compare = std::less<K>>
using TSet = std::set<K, Compare, TEngineAllocator<K>>;

/// 哈希映射
template<typename K, typename V,
         typename Hash = std::hash<K>,
         typename KeyEqual = std::equal_to<K>>
using THashMap = std::unordered_map<K, V, Hash, KeyEqual,
                                    TEngineAllocator<std::pair<const K, V>>>;

/// 哈希集合
template<typename K,
         typename Hash = std::hash<K>,
         typename KeyEqual = std::equal_to<K>>
using THashSet = std::unordered_set<K, Hash, KeyEqual, TEngineAllocator<K>>;

} // namespace TE
