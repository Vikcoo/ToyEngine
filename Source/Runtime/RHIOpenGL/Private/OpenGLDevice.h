// ToyEngine RHIOpenGL Module
// OpenGL Device 实现 - 封装 OpenGL 资源创建

#pragma once

#include "RHIDevice.h"
#include "RHITransientAllocator.h"

namespace TE {

class OpenGLCommandBuffer;

/// OpenGL 后端的 Device 实现
/// 负责创建所有 OpenGL GPU 资源
class OpenGLDevice final : public RHIDevice
{
public:
    explicit OpenGLDevice(const RHIDeviceCreateDesc& desc);
    ~OpenGLDevice() override;

    // 禁止拷贝
    OpenGLDevice(const OpenGLDevice&) = delete;
    OpenGLDevice& operator=(const OpenGLDevice&) = delete;

    [[nodiscard]] RHIFrameStatus BeginFrame(const RHIFrameBeginInfo& beginInfo,
                                            RHIFrameContext& outContext) override;
    [[nodiscard]] RHIFrameStatus EndFrame(RHIFrameContext& context) override;
    void WaitIdle() override;
    [[nodiscard]] bool AllocateTransientUniform(const void* data,
                                                uint64_t size,
                                                RHITransientUniformAllocation& outAllocation) override;

    [[nodiscard]] std::unique_ptr<RHIBuffer> CreateBuffer(const RHIBufferDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIShader> CreateShader(const RHIShaderDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIPipeline> CreatePipeline(const RHIPipelineDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHICommandBuffer> CreateCommandBuffer() override;
    [[nodiscard]] std::unique_ptr<RHITexture> CreateTexture(const RHITextureDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHISampler> CreateSampler(const RHISamplerDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIBindGroup> CreateBindGroup(const RHIBindGroupDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIBindGroupLayout> CreateBindGroupLayout(const RHIBindGroupLayoutDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIPipelineLayout> CreatePipelineLayout(const RHIPipelineLayoutDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIRenderTarget> CreateRenderTarget(const RHIRenderTargetDesc& desc) override;
    [[nodiscard]] const RHIBackendTraits& GetBackendTraits() const override;
    [[nodiscard]] RHIFormat GetBackBufferColorFormat() const override { return RHIFormat::RGBA8_UNorm; }
    [[nodiscard]] RHIFormat GetBackBufferDepthFormat() const override { return RHIFormat::D24_UNorm_S8_UInt; }
    [[nodiscard]] Matrix4 AdjustProjectionMatrix(const Matrix4& projection) const override;

private:
    RHIBackendTraits m_Traits;
    std::unique_ptr<OpenGLCommandBuffer> m_FrameCommandBuffer;
    std::unique_ptr<RHIBuffer> m_TransientUniformBuffer;
    RHITransientRangeAllocator m_TransientUniformAllocator;
    void* m_PlatformUserData = nullptr;
    RHIPlatformPresentCallback m_PlatformPresent = nullptr;
    uint64_t m_FrameNumber = 0;
    uint32_t m_FramesInFlight = 1;
    bool m_FrameActive = false;
};

} // namespace TE
