// ToyEngine Renderer Module
// FStaticMeshSceneProxy - 静态网格的渲染侧镜像
// 对应 UE5 的 FStaticMeshSceneProxy
//
// 负责管理静态网格的 GPU 资源（VBO、IBO、Shader、Pipeline），
// 在构造时通过 RHIDevice 创建这些资源

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
struct RHIVertexInputDesc;

/// 静态网格数据描述（从 TMeshComponent 传递给 Proxy）
/// 包含创建 GPU 资源所需的全部 CPU 侧数据
struct FStaticMeshData
{
    std::vector<float>      Vertices;       // 顶点数据（交错布局：Position + Color）
    std::vector<uint32_t>   Indices;        // 索引数据
    uint32_t                VertexStride = 0;  // 每个顶点的字节步长
    std::string             VertexShaderPath;  // 顶点着色器路径
    std::string             FragmentShaderPath; // 片段着色器路径

    // 顶点属性布局
    struct VertexAttribute
    {
        uint32_t location;      // Shader 中的 location
        uint32_t format;        // 对应 RHIFormat 的值
        uint32_t offset;        // 在顶点中的字节偏移
    };
    std::vector<VertexAttribute> Attributes;

    // Pipeline 状态
    bool DepthTestEnabled = true;
    bool DepthWriteEnabled = true;
    bool BackfaceCulling = true;
};

/// 静态网格渲染侧 Proxy
///
/// UE5 映射：
/// - FStaticMeshSceneProxy: 继承 FPrimitiveSceneProxy
/// - 在构造时从 UStaticMesh 获取 LOD 数据，创建对应的 RenderBatch
///
/// 在 ToyEngine 中：
/// - 构造时接收 FStaticMeshData + RHIDevice
/// - 通过 RHIDevice 创建 VBO/IBO/Shader/Pipeline
/// - GetMeshDrawCommand() 填充绘制命令
class FStaticMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
    /// 构造函数：接收网格数据，通过 RHIDevice 创建 GPU 资源
    FStaticMeshSceneProxy(const FStaticMeshData& meshData, RHIDevice* device);
    ~FStaticMeshSceneProxy() override = default;

    /// 收集绘制命令
    [[nodiscard]] bool GetMeshDrawCommand(FMeshDrawCommand& outCmd) const override;

    /// GPU 资源是否创建成功
    [[nodiscard]] bool IsValid() const;

private:
    // RHI 资源（Proxy 拥有这些资源的生命周期）
    std::unique_ptr<RHIBuffer>      m_VertexBuffer;
    std::unique_ptr<RHIShader>      m_VertexShader;
    std::unique_ptr<RHIShader>      m_FragmentShader;
    std::unique_ptr<RHIPipeline>    m_Pipeline;
    std::unique_ptr<RHIBuffer>      m_IndexBuffer;

    uint32_t m_IndexCount = 0;
};

} // namespace TE
