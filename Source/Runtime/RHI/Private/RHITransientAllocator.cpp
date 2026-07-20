// ToyEngine RHI Module
// 帧内瞬态范围分配器实现

#include "RHITransientAllocator.h"

#include <limits>

namespace TE {

bool RHITransientRangeAllocator::Initialize(const uint64_t bytesPerFrame,
                                            const uint32_t frameCount,
                                            const uint64_t alignment)
{
    if (bytesPerFrame == 0 || frameCount == 0 || alignment == 0 ||
        bytesPerFrame > std::numeric_limits<uint64_t>::max() / frameCount)
    {
        return false;
    }

    m_BytesPerFrame = bytesPerFrame;
    m_FrameCount = frameCount;
    m_Alignment = alignment;
    m_FrameBase = 0;
    m_Cursor = 0;
    return true;
}

void RHITransientRangeAllocator::BeginFrame(const uint32_t frameIndex)
{
    if (m_FrameCount == 0)
    {
        m_FrameBase = 0;
        m_Cursor = 0;
        return;
    }

    m_FrameBase = static_cast<uint64_t>(frameIndex % m_FrameCount) * m_BytesPerFrame;
    m_Cursor = 0;
}

bool RHITransientRangeAllocator::Allocate(const uint64_t size, uint64_t& outOffset)
{
    outOffset = 0;
    if (size == 0 || m_BytesPerFrame == 0)
    {
        return false;
    }

    const uint64_t alignedCursor = AlignUp(m_Cursor, m_Alignment);
    if (alignedCursor > m_BytesPerFrame || size > m_BytesPerFrame - alignedCursor)
    {
        return false;
    }

    outOffset = m_FrameBase + alignedCursor;
    m_Cursor = alignedCursor + size;
    return true;
}

uint64_t RHITransientRangeAllocator::AlignUp(const uint64_t value, const uint64_t alignment)
{
    const uint64_t remainder = value % alignment;
    if (remainder == 0)
    {
        return value;
    }

    const uint64_t padding = alignment - remainder;
    if (value > std::numeric_limits<uint64_t>::max() - padding)
    {
        return std::numeric_limits<uint64_t>::max();
    }
    return value + padding;
}

} // namespace TE
