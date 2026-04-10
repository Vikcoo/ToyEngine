// ToyEngine Renderer Module
// FStaticMeshSceneProxy - 静态网格的渲染侧镜像
// 对应 UE5 的 FStaticMeshSceneProxy
//
// 重构说明：
// - 支持多 Section（每个 Section 对应一个 VBO+IBO）
// - 从 TStaticMesh 资产读取数据创建 GPU 资源
// - GetMeshDrawCommands() 返回多条绘制命令（每个 Section 一条）

#pragma once

#include "FPrimitiveSceneProxy.h"
#include <memory>
#include <vector>
#include <string>
#include <cstdint>

namespace TE {

// 前向声明
class RHIDevice;
class RHIBuffer;
class RHIShader;
class RHIPipeline;
class TStaticMesh;
struct RHIVertexInputDesc;

/// 每个 Section 的 GPU 资源
/// 对应 UE5 中 FStaticMeshBatch / FMeshBatchElement
struct FSectionGPUData
{
    std::unique_ptr<RHIBuffer> VertexBuffer;
    std::unique_ptr<RHIBuffer> IndexBuffer;
    uint32_t IndexCount = 0;
};

/// 静态网格渲染侧 Proxy
///
/// UE5 映射：
/// - FStaticMeshSceneProxy: 继承 FPrimitiveSceneProxy
/// - 在构造时从 UStaticMesh 获取 LOD 数据，创建对应的 RenderBatch
///
/// 在 ToyEngine 中：
/// - 构造时接收 TStaticMesh + RHIDevice
/// - 为每个 Section 创建独立的 VBO/IBO
/// - 共享同一套 Shader/Pipeline（所有 Section 用相同的 model shader）
/// - GetMeshDrawCommands() 为每个 Section 填充一条绘制命令
class FStaticMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
    /// 构造函数：从 TStaticMesh 资产创建 GPU 资源
    FStaticMeshSceneProxy(const TStaticMesh* staticMesh, RHIDevice* device);
    ~FStaticMeshSceneProxy() override;

    /// 收集绘制命令（每个 Section 一条）
    void GetMeshDrawCommands(std::vector<FMeshDrawCommand>& outCommands) const override;

    /// GPU 资源是否创建成功
    [[nodiscard]] bool IsValid() const;

private:
    /// 创建共享的 Shader 和 Pipeline
    [[nodiscard]] bool CreateShaderAndPipeline(RHIDevice* device);

    // 每个 Section 的 GPU 资源
    std::vector<FSectionGPUData>    m_SectionGPUData;

    // 所有 Section 共享的 Shader 和 Pipeline
    std::unique_ptr<RHIShader>      m_VertexShader;
    std::unique_ptr<RHIShader>      m_FragmentShader;
    std::unique_ptr<RHIPipeline>    m_Pipeline;
};

} // namespace TE
