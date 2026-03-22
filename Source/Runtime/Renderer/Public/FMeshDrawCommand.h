// ToyEngine Renderer Module
// FMeshDrawCommand - 网格绘制命令
// 对应 UE5 的 FMeshDrawCommand（简化版）
// 包含渲染一个 Mesh 所需的全部 RHI 绑定信息

#pragma once

#include "Math/MathTypes.h"
#include <cstdint>

namespace TE {

// 前向声明（避免 Renderer 模块硬依赖 RHI 具体头文件）
class RHIPipeline;
class RHIBuffer;

/// 网格绘制命令
///
/// UE5 映射：
/// - FMeshDrawCommand: 存储 Pipeline + VBO/IBO + Uniform 数据 + 绘制参数
/// - SceneRenderer 通过收集 FMeshDrawCommand 实现渲染排序和批处理
///
/// 在单线程版本中，SceneProxy::GetMeshDrawCommand() 直接填充此结构体，
/// SceneRenderer 收集后统一提交给 RHI
struct FMeshDrawCommand
{
    RHIPipeline*    Pipeline = nullptr;         // 图形管线（Shader + 状态）
    RHIBuffer*      VertexBuffer = nullptr;     // 顶点缓冲区
    RHIBuffer*      IndexBuffer = nullptr;      // 索引缓冲区
    uint32_t        IndexCount = 0;             // 索引数量
    Matrix4         WorldMatrix;                // 物体的世界变换矩阵（Model 矩阵）
};

} // namespace TE
