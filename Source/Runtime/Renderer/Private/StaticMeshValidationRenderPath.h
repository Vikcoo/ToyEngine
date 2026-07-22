// ToyEngine Renderer Module
// Vulkan 阶段 B 静态网格能力验证路径

#pragma once

#include "RendererTransientUniforms.h"

#include <memory>

namespace TE {

class RHIBindGroup;
class RHIBindGroupLayout;
class RHIBuffer;
class RHICommandBuffer;
class RHIDevice;
class RHIPipeline;
class RHIPipelineLayout;
class RHISampler;
class RHIShader;
class RHITexture;
struct FRenderStats;

/** 验证阶段 B 的静态网格、纹理、Descriptor、动态 Uniform 与深度路径。 */
class FStaticMeshValidationRenderPath final
{
public:
    FStaticMeshValidationRenderPath();
    ~FStaticMeshValidationRenderPath();

    FStaticMeshValidationRenderPath(const FStaticMeshValidationRenderPath&) = delete;
    FStaticMeshValidationRenderPath& operator=(const FStaticMeshValidationRenderPath&) = delete;

    void Render(RHIDevice* device, RHICommandBuffer* commandBuffer, FRenderStats& outStats);

private:
    [[nodiscard]] bool EnsureResources(RHIDevice* device);

    RHIDevice* m_Device = nullptr;
    std::unique_ptr<RHIBuffer> m_VertexBuffer;
    std::unique_ptr<RHIBuffer> m_IndexBuffer;
    std::unique_ptr<RHITexture> m_Texture;
    std::unique_ptr<RHISampler> m_Sampler;
    std::unique_ptr<RHIShader> m_VertexShader;
    std::unique_ptr<RHIShader> m_FragmentShader;
    std::unique_ptr<RHIBindGroupLayout> m_ObjectLayout;
    std::unique_ptr<RHIBindGroupLayout> m_TextureLayout;
    std::unique_ptr<RHIPipelineLayout> m_PipelineLayout;
    std::unique_ptr<RHIBindGroup> m_TextureBindGroup;
    std::unique_ptr<RHIPipeline> m_Pipeline;
    FTransientUniformBindingState m_ObjectBindingState;
    float m_Rotation = 0.0f;
};

} // namespace TE
