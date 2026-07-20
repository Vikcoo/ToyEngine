// ToyEngine RHI Module
// 帧内瞬态范围分配器 - 为 Uniform ring 提供后端无关的对齐与分段算法

#pragma once

#include <cstdint>

namespace TE {

class RHITransientRangeAllocator
{
public:
    [[nodiscard]] bool Initialize(uint64_t bytesPerFrame, uint32_t frameCount, uint64_t alignment);
    void BeginFrame(uint32_t frameIndex);
    [[nodiscard]] bool Allocate(uint64_t size, uint64_t& outOffset);

    [[nodiscard]] uint64_t GetTotalSize() const { return m_BytesPerFrame * m_FrameCount; }
    [[nodiscard]] uint64_t GetBytesPerFrame() const { return m_BytesPerFrame; }
    [[nodiscard]] uint64_t GetAlignment() const { return m_Alignment; }

private:
    [[nodiscard]] static uint64_t AlignUp(uint64_t value, uint64_t alignment);

    uint64_t m_BytesPerFrame = 0;
    uint64_t m_Alignment = 1;
    uint64_t m_FrameBase = 0;
    uint64_t m_Cursor = 0;
    uint32_t m_FrameCount = 0;
};

} // namespace TE
