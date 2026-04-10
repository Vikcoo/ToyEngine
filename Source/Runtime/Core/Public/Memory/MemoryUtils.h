// ToyEngine Core Module
// 内存工具函数声明

#pragma once

#include "Memory.h"

#include <cstddef>

namespace TE {

// ============================================================
// MemoryTagName — 获取 MemoryTag 的可读名称
// ============================================================

const char* MemoryTagName(MemoryTag tag);

// ============================================================
// 字节格式化辅助
// ============================================================

struct FormattedBytes
{
    double Value;
    const char* Unit;
};

FormattedBytes FormatBytes(std::uint64_t bytes);

// ============================================================
// DumpMemoryStats — 打印完整内存统计到日志
// ============================================================

void DumpMemoryStats();

} // namespace TE
