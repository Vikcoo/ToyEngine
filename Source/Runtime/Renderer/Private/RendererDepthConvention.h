// ToyEngine Renderer Module
// RendererDepthConvention - GPU 深度方向约定

#pragma once

#include "Math/Matrix.h"
#include "RHITypes.h"

namespace TE::RendererDepth {

inline constexpr float ClearValue = 0.0f;
inline constexpr RHICompareOp CompareOp = RHICompareOp::Greater;

/** 将 CPU 正向 ZO 投影转换为 Renderer 使用的 Reversed-Z 投影。 */
[[nodiscard]] inline Matrix4 BuildProjection(const Matrix4& projectionZO)
{
    return Matrix4::ReverseZProjectionZO(projectionZO);
}

} // namespace TE::RendererDepth
