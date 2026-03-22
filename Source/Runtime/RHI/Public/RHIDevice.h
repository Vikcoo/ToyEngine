// ToyEngine RHI Module
// RHI Device 抽象接口 - GPU 设备抽象，负责创建所有 GPU 资源
// 设计参考 Vulkan VkDevice / D3D12 ID3D12Device

#pragma once

#include "RHITypes.h"
#include <memory>

namespace TE {

/// GPU 设备抽象接口
///
/// Device 是 RHI 的核心对象，负责创建所有 GPU 资源：
/// - Buffer（顶点/索引/Uniform 缓冲区）
/// - Shader（着色器模块）
/// - Pipeline（图形管线状态对象）
/// - CommandBuffer（命令缓冲区）
///
/// 每个图形 API 后端提供一个 Device 实现：
/// - OpenGLDevice: 封装 OpenGL 上下文中的资源创建
/// - VulkanDevice: 封装 VkDevice 的资源创建（未来）
/// - D3D12Device:  封装 ID3D12Device 的资源创建（未来）
class RHIDevice
{
public:
    virtual ~RHIDevice() = default;

    /// 创建 GPU 缓冲区
    [[nodiscard]] virtual std::unique_ptr<RHIBuffer> CreateBuffer(const RHIBufferDesc& desc) = 0;

    /// 创建着色器模块
    [[nodiscard]] virtual std::unique_ptr<RHIShader> CreateShader(const RHIShaderDesc& desc) = 0;

    /// 创建图形管线
    [[nodiscard]] virtual std::unique_ptr<RHIPipeline> CreatePipeline(const RHIPipelineDesc& desc) = 0;

    /// 创建命令缓冲区
    [[nodiscard]] virtual std::unique_ptr<RHICommandBuffer> CreateCommandBuffer() = 0;

    /// 工厂方法：根据编译选项（TE_RHI_OPENGL / TE_RHI_VULKAN 等）创建对应后端的 Device
    [[nodiscard]] static std::unique_ptr<RHIDevice> Create();

protected:
    RHIDevice() = default;
};

} // namespace TE
