// ToyEngine Scene Module
// FViewInfo - 视图信息
// 由游戏侧相机构建，供渲染侧消费

#pragma once

#include "Math/MathTypes.h"

namespace TE {

/// 视图信息（由 CameraComponent 构建，传递给渲染器）
struct FViewInfo
{
    Matrix4 ViewMatrix;           // 世界空间 -> 相机空间
    Matrix4 ProjectionMatrix;     // 相机空间 -> 裁剪空间（NDC）
    Matrix4 ViewProjectionMatrix; // Projection * View 的预乘结果

    float ViewportWidth = 1280.0f;  // 视口宽度
    float ViewportHeight = 720.0f;  // 视口高度

    void UpdateViewProjectionMatrix()
    {
        ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
    }
};

} // namespace TE
