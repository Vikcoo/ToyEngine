// ToyEngine RHI Module
// RHI RenderTarget 抽象接口 - 封装渲染目标（离屏 / 交换链帧缓冲）
// 对应 Vulkan VkFramebuffer + VkRenderPass / D3D12 RTV + DSV / OpenGL FBO

#pragma once

#include "RHITypes.h"

namespace TE {

/// 渲染目标附件描述
struct RHIAttachmentDesc
{
    RHIFormat format = RHIFormat::RGBA8_UNorm;
    bool      isDepthStencil = false;
};

/// 渲染目标创建描述
struct RHIRenderTargetDesc
{
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<RHIAttachmentDesc> colorAttachments;
    RHIAttachmentDesc              depthStencilAttachment;
    bool                           hasDepthStencil = true;
    std::string                    debugName;
};

/// 渲染目标抽象接口
/// 封装离屏渲染或交换链帧缓冲。当前用于为后续延迟渲染、后处理提供 MRT 基础设施。
class RHIRenderTarget
{
public:
    virtual ~RHIRenderTarget() = default;

    [[nodiscard]] virtual bool IsValid() const = 0;
    [[nodiscard]] virtual uint32_t GetWidth() const = 0;
    [[nodiscard]] virtual uint32_t GetHeight() const = 0;

    /// 获取指定 color attachment 对应的纹理（可用于后续 Pass 采样）
    [[nodiscard]] virtual RHITexture* GetColorAttachment(uint32_t index) const = 0;

    /// 获取深度/模板附件纹理
    [[nodiscard]] virtual RHITexture* GetDepthStencilAttachment() const = 0;

protected:
    RHIRenderTarget() = default;
};

} // namespace TE
