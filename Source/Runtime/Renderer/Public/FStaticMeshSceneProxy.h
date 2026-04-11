// ToyEngine Renderer Module
// FStaticMeshSceneProxy - 静态网格的渲染侧镜像
// 对应 UE5 的 FStaticMeshSceneProxy
//
// 重构说明：
// - 支持多 Section（每个 Section 对应一个索引范围）
// - 只引用资产级 FStaticMeshRenderData，不负责 GPU 资源创建
// - GetMeshDrawCommands() 返回多条绘制命令（每个 Section 一条）

#pragma once

#include "FPrimitiveSceneProxy.h"
#include "FStaticMeshRenderData.h"

#include <memory>

namespace TE {

// 前向声明
class RHIPipeline;

/// 静态网格渲染侧 Proxy
///
/// UE5 映射：
/// - FStaticMeshSceneProxy: 继承 FPrimitiveSceneProxy
/// - 在构造时绑定已准备好的 RenderData
///
/// 在 ToyEngine 中：
/// - 构造时接收 FStaticMeshRenderData + RHIPipeline
/// - SceneProxy 仅描述实例级渲染信息，不创建 GPU 资源
/// - GetMeshDrawCommands() 为每个 Section 填充一条绘制命令
class FStaticMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
    /// 构造函数：绑定资产级渲染数据与管线
    FStaticMeshSceneProxy(std::shared_ptr<const FStaticMeshRenderData> renderData, RHIPipeline* pipeline);
    ~FStaticMeshSceneProxy() override;

    /// 收集绘制命令（每个 Section 一条）
    void GetMeshDrawCommands(std::vector<FMeshDrawCommand>& outCommands) const override;

    /// 引用的渲染数据是否有效
    [[nodiscard]] bool IsValid() const;

private:
    std::shared_ptr<const FStaticMeshRenderData> m_RenderData;
    RHIPipeline* m_Pipeline = nullptr;
};

} // namespace TE
