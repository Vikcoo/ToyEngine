// ToyEngine RHI Module
// RHI Pipeline 抽象接口 - 封装图形管线状态对象

#pragma once

#include "RHITypes.h"

namespace TE {

/// 图形管线状态对象抽象接口
/// 对应 Vulkan VkPipeline / D3D12 ID3D12PipelineState / OpenGL Program + VAO + 状态集
///
/// Pipeline 封装了完整的图形管线状态：
/// - 着色器组合（Vertex + Fragment）
/// - 顶点输入布局
/// - 图元拓扑
/// - 光栅化状态（多边形模式、剔除、正面定义）
/// - 深度/模板状态
class RHIPipeline
{
public:
    virtual ~RHIPipeline() = default;

    /// 管线是否有效
    [[nodiscard]] virtual bool IsValid() const = 0;

protected:
    RHIPipeline() = default;
};

} // namespace TE
