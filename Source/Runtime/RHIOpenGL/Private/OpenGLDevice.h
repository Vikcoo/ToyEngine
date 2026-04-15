// ToyEngine RHIOpenGL Module
// OpenGL Device 实现 - 封装 OpenGL 资源创建

#pragma once

#include "RHIDevice.h"

namespace TE {

/// OpenGL 后端的 Device 实现
/// 负责创建所有 OpenGL GPU 资源
class OpenGLDevice final : public RHIDevice
{
public:
    OpenGLDevice();
    ~OpenGLDevice() override;

    // 禁止拷贝
    OpenGLDevice(const OpenGLDevice&) = delete;
    OpenGLDevice& operator=(const OpenGLDevice&) = delete;

    [[nodiscard]] std::unique_ptr<RHIBuffer> CreateBuffer(const RHIBufferDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIShader> CreateShader(const RHIShaderDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIPipeline> CreatePipeline(const RHIPipelineDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHICommandBuffer> CreateCommandBuffer() override;
    [[nodiscard]] std::unique_ptr<RHITexture> CreateTexture(const RHITextureDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHISampler> CreateSampler(const RHISamplerDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIBindGroup> CreateBindGroup(const RHIBindGroupDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIRenderTarget> CreateRenderTarget(const RHIRenderTargetDesc& desc) override;
    [[nodiscard]] const RHIBackendTraits& GetBackendTraits() const override;
    [[nodiscard]] Matrix4 AdjustProjectionMatrix(const Matrix4& projection) const override;

private:
    RHIBackendTraits m_Traits;
};

} // namespace TE
