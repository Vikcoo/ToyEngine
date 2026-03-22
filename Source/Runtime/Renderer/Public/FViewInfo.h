// ToyEngine Renderer Module
// FViewInfo - 视图信息
// 对应 UE5 的 FSceneView / FViewInfo（简化版）
// 包含相机的 View 矩阵、Projection 矩阵及视口参数

#pragma once

#include "Math/MathTypes.h"

namespace TE {

/// 视图信息（由 CameraComponent 构建，传递给 SceneRenderer）
///
/// UE5 映射：
/// - FSceneView: 包含 ViewMatrix、ProjectionMatrix、ViewProjectionMatrix
/// - FViewInfo: 继承 FSceneView，增加渲染相关数据
///
/// 单线程版本中，CameraComponent 在每帧 BuildViewInfo() 时直接构建此结构体
struct FViewInfo
{
    Matrix4     ViewMatrix;                 // 世界空间 → 相机空间
    Matrix4     ProjectionMatrix;           // 相机空间 → 裁剪空间（NDC）
    Matrix4     ViewProjectionMatrix;       // View * Projection 的预乘结果

    float       ViewportWidth = 1280.0f;    // 视口宽度
    float       ViewportHeight = 720.0f;    // 视口高度

    /// 更新 ViewProjectionMatrix（View 或 Projection 变化时调用）
    void UpdateViewProjectionMatrix()
    {
        ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
    }
};

} // namespace TE
