// ToyEngine RHI Module
// RHI Device 抽象接口 - GPU 设备抽象，负责创建所有 GPU 资源
// 设计参考 Vulkan VkDevice / D3D12 ID3D12Device

#pragma once

#include "RHITypes.h"
#include "RHIRenderTarget.h"
#include "Math/MathTypes.h"
#include <memory>

namespace TE {

/**
 * GPU 设备抽象接口。
 *
 * Device 负责帧生命周期以及 Buffer、Shader、Pipeline、CommandBuffer 等 GPU 资源创建。
 * OpenGLDevice 已实现完整场景路径；VulkanDevice 阶段 B 已实现基础资源、Descriptor 与静态网格验证路径；D3D12Device 尚未实现。
 */
class RHIDevice
{
public:
    virtual ~RHIDevice() = default;

    /** 开始一帧并返回由后端管理的当前帧 CommandBuffer。 */
    [[nodiscard]] virtual RHIFrameStatus BeginFrame(const RHIFrameBeginInfo& beginInfo,
                                                    RHIFrameContext& outContext) = 0;

    /** 结束录制、提交并呈现当前帧。 */
    [[nodiscard]] virtual RHIFrameStatus EndFrame(RHIFrameContext& context) = 0;

    /** 等待该 Device 已提交的 GPU 工作完成。 */
    virtual void WaitIdle() = 0;

    /**
     * 从当前帧的瞬态 Uniform ring 分配一段稳定范围并写入数据。
     * @note 返回范围只保证存活到对应 frame fence 完成；只能在 BeginFrame/EndFrame 之间调用。
     */
    [[nodiscard]] virtual bool AllocateTransientUniform(const void* data,
                                                        uint64_t size,
                                                        RHITransientUniformAllocation& outAllocation) = 0;

    /// 创建 GPU 缓冲区
    [[nodiscard]] virtual std::unique_ptr<RHIBuffer> CreateBuffer(const RHIBufferDesc& desc) = 0;

    /// 创建着色器模块
    [[nodiscard]] virtual std::unique_ptr<RHIShader> CreateShader(const RHIShaderDesc& desc) = 0;

    /// 创建图形管线
    [[nodiscard]] virtual std::unique_ptr<RHIPipeline> CreatePipeline(const RHIPipelineDesc& desc) = 0;

    /// 创建命令缓冲区
    [[nodiscard]] virtual std::unique_ptr<RHICommandBuffer> CreateCommandBuffer() = 0;

    /// 创建 2D 纹理
    [[nodiscard]] virtual std::unique_ptr<RHITexture> CreateTexture(const RHITextureDesc& desc) = 0;

    /// 创建纹理采样器
    [[nodiscard]] virtual std::unique_ptr<RHISampler> CreateSampler(const RHISamplerDesc& desc) = 0;

    /// 创建资源绑定组（Vulkan: DescriptorSet, D3D12: Descriptor Table, OpenGL: UBO+Texture 绑定集）
    [[nodiscard]] virtual std::unique_ptr<RHIBindGroup> CreateBindGroup(const RHIBindGroupDesc& desc) = 0;

    /// 创建资源绑定布局（Vulkan: DescriptorSetLayout, D3D12: Descriptor Table / Root Parameter 描述）
    [[nodiscard]] virtual std::unique_ptr<RHIBindGroupLayout> CreateBindGroupLayout(const RHIBindGroupLayoutDesc& desc) = 0;

    /// 创建 Pipeline 资源布局（Vulkan: PipelineLayout, D3D12: RootSignature）
    [[nodiscard]] virtual std::unique_ptr<RHIPipelineLayout> CreatePipelineLayout(const RHIPipelineLayoutDesc& desc) = 0;

    /// 创建渲染目标（离屏 FBO / MRT）
    [[nodiscard]] virtual std::unique_ptr<RHIRenderTarget> CreateRenderTarget(const RHIRenderTargetDesc& desc) = 0;

    /// 查询当前后端的特征描述（NDC 深度范围、纹理原点、Y 轴方向等）
    [[nodiscard]] virtual const RHIBackendTraits& GetBackendTraits() const = 0;

    [[nodiscard]] virtual RHIFormat GetBackBufferColorFormat() const = 0;
    [[nodiscard]] virtual RHIFormat GetBackBufferDepthFormat() const = 0;

    /// 对引擎约定的 [0,1] 深度、Y-up 投影矩阵做后端适配。
    /// 引擎上层生成的投影矩阵统一为 [0,1] 深度 + Y-up，此方法在提交渲染前由后端修正：
    ///   - OpenGL 未启用 glClipControl 时：将 [0,1] 重映射为 [-1,1]
    ///   - Vulkan：翻转 Y 轴（NDC Y 向下）
    ///   - D3D12 / OpenGL 已启用 glClipControl：原样返回
    [[nodiscard]] virtual Matrix4 AdjustProjectionMatrix(const Matrix4& projection) const;

    /// 工厂方法：根据编译选项（TE_RHI_OPENGL / TE_RHI_VULKAN 等）创建对应后端的 Device
    [[nodiscard]] static std::unique_ptr<RHIDevice> Create(const RHIDeviceCreateDesc& desc);

protected:
    RHIDevice() = default;
};

} // namespace TE
