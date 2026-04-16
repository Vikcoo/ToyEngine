// ToyEngine RHIOpenGL Module
// OpenGL RenderTarget 实现 - 封装 OpenGL FBO（离屏帧缓冲 / MRT）

#pragma once

#include "RHIRenderTarget.h"
#include "OpenGLTexture.h"

#include <glad/glad.h>

#include <memory>
#include <vector>

namespace TE {

/// OpenGL 后端的 RenderTarget 实现。
///
/// 职责：
/// - 为 `RHIRenderTargetDesc` 中每个 color attachment 创建一个 `OpenGLTexture`，
///   并通过 `glFramebufferTexture2D` 绑定到一个 `GLuint` FBO。
/// - 可选创建深度/深度模板附件纹理。
/// - 创建后执行 `glCheckFramebufferStatus` 验证完整性。
/// - 生命周期内保留所有附件纹理的所有权，外部通过 `GetColorAttachment / GetDepthStencilAttachment`
///   以 `RHITexture*` 形式获取，用于后续 Pass（例如 Deferred 的 Lighting Pass）采样。
class OpenGLRenderTarget final : public RHIRenderTarget
{
public:
    explicit OpenGLRenderTarget(const RHIRenderTargetDesc& desc);
    ~OpenGLRenderTarget() override;

    OpenGLRenderTarget(const OpenGLRenderTarget&) = delete;
    OpenGLRenderTarget& operator=(const OpenGLRenderTarget&) = delete;

    [[nodiscard]] bool IsValid() const override { return m_FBO != 0 && m_IsComplete; }
    [[nodiscard]] uint32_t GetWidth() const override { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const override { return m_Height; }

    [[nodiscard]] RHITexture* GetColorAttachment(uint32_t index) const override;
    [[nodiscard]] RHITexture* GetDepthStencilAttachment() const override;

    /// 获取底层 FBO 句柄，供 `OpenGLCommandBuffer::BeginRenderPass` 绑定使用。
    [[nodiscard]] GLuint GetGLFramebufferID() const { return m_FBO; }

    /// 获取 color attachment 数量，供 `glDrawBuffers` 启用使用。
    [[nodiscard]] uint32_t GetColorAttachmentCount() const { return static_cast<uint32_t>(m_ColorAttachments.size()); }

private:
    GLuint   m_FBO = 0;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    bool     m_IsComplete = false;

    std::vector<std::unique_ptr<OpenGLTexture>> m_ColorAttachments;
    std::unique_ptr<OpenGLTexture>              m_DepthStencilAttachment;
};

} // namespace TE
