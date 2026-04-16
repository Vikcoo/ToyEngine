// ToyEngine RHIOpenGL Module
// OpenGL RenderTarget 实现 - 封装 FBO + MRT

#include "OpenGLRenderTarget.h"

#include "Log/Log.h"

namespace TE {

namespace {

/// 将 RHIRenderTargetDesc 中的单个附件描述转换成创建 OpenGLTexture 用的 RHITextureDesc。
/// 附件纹理统一不生成 mipmap，不携带初始数据，sRGB 标志随彩色附件格式推断。
RHITextureDesc MakeAttachmentTextureDesc(uint32_t width, uint32_t height, const RHIAttachmentDesc& attach, const std::string& debugBase, uint32_t indexForDebug)
{
    RHITextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = attach.format;
    desc.initialData = nullptr;
    desc.generateMips = false;

    // sRGB 仅对彩色附件有效；深度附件强制 false。OpenGLTexture 内部会根据深度格式
    // 进一步跳过 srgb 处理，这里的字段仅作为彩色附件提示。
    const bool isSRGBColor = (attach.format == RHIFormat::RGB8_sRGB || attach.format == RHIFormat::RGBA8_sRGB);
    desc.srgb = (!attach.isDepthStencil) && isSRGBColor;

    desc.debugName = debugBase + (attach.isDepthStencil ? "_Depth" : ("_Color" + std::to_string(indexForDebug)));
    return desc;
}

} // namespace

OpenGLRenderTarget::OpenGLRenderTarget(const RHIRenderTargetDesc& desc)
    : m_Width(desc.width)
    , m_Height(desc.height)
{
    if (desc.width == 0 || desc.height == 0)
    {
        TE_LOG_ERROR("[RHIOpenGL] OpenGLRenderTarget create failed: invalid size {}x{}", desc.width, desc.height);
        return;
    }

    glGenFramebuffers(1, &m_FBO);
    if (m_FBO == 0)
    {
        TE_LOG_ERROR("[RHIOpenGL] OpenGLRenderTarget create failed: glGenFramebuffers returned 0");
        return;
    }

    // 保存当前绑定的 FBO，创建完成后恢复，避免污染上层命令录制中的 FBO 状态。
    GLint previousDrawFBO = 0;
    GLint previousReadFBO = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFBO);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFBO);

    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    // 创建并绑定所有彩色附件
    m_ColorAttachments.reserve(desc.colorAttachments.size());
    for (uint32_t i = 0; i < desc.colorAttachments.size(); ++i)
    {
        auto attachDesc = MakeAttachmentTextureDesc(desc.width, desc.height, desc.colorAttachments[i], desc.debugName, i);
        auto tex = std::make_unique<OpenGLTexture>(attachDesc);
        if (!tex->IsValid())
        {
            TE_LOG_ERROR("[RHIOpenGL] RenderTarget color attachment[{}] create failed", i);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previousDrawFBO));
            glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousReadFBO));
            return;
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0 + i,
                               GL_TEXTURE_2D,
                               tex->GetGLTextureID(),
                               0);

        m_ColorAttachments.push_back(std::move(tex));
    }

    // 创建并绑定深度附件（可选）
    if (desc.hasDepthStencil)
    {
        auto attachDesc = MakeAttachmentTextureDesc(desc.width, desc.height, desc.depthStencilAttachment, desc.debugName, 0);
        attachDesc.debugName = desc.debugName + "_Depth";

        auto tex = std::make_unique<OpenGLTexture>(attachDesc);
        if (!tex->IsValid())
        {
            TE_LOG_ERROR("[RHIOpenGL] RenderTarget depth attachment create failed");
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previousDrawFBO));
            glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousReadFBO));
            return;
        }

        // 根据格式选择合适的 attachment 点（纯深度 vs 深度+模板）
        const GLenum attachmentPoint = (desc.depthStencilAttachment.format == RHIFormat::D24_UNorm_S8_UInt)
            ? GL_DEPTH_STENCIL_ATTACHMENT
            : GL_DEPTH_ATTACHMENT;

        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               attachmentPoint,
                               GL_TEXTURE_2D,
                               tex->GetGLTextureID(),
                               0);

        m_DepthStencilAttachment = std::move(tex);
    }

    // 如果没有彩色附件（例如 ShadowMap 只写深度），需要显式通知 OpenGL 无绘制 buffer，否则 FBO 不完整
    if (m_ColorAttachments.empty())
    {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        TE_LOG_ERROR("[RHIOpenGL] RenderTarget '{}' incomplete, status=0x{:X}", desc.debugName, static_cast<uint32_t>(status));
    }
    else
    {
        m_IsComplete = true;
    }

    // 恢复之前的 FBO 绑定状态
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previousDrawFBO));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousReadFBO));
}

OpenGLRenderTarget::~OpenGLRenderTarget()
{
    // 附件纹理由 unique_ptr 成员自动释放
    if (m_FBO != 0)
    {
        glDeleteFramebuffers(1, &m_FBO);
        m_FBO = 0;
    }
}

RHITexture* OpenGLRenderTarget::GetColorAttachment(uint32_t index) const
{
    if (index >= m_ColorAttachments.size())
    {
        return nullptr;
    }
    return m_ColorAttachments[index].get();
}

RHITexture* OpenGLRenderTarget::GetDepthStencilAttachment() const
{
    return m_DepthStencilAttachment.get();
}

} // namespace TE
