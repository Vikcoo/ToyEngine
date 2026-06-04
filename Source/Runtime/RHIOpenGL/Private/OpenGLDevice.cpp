// ToyEngine RHIOpenGL Module
// OpenGL Device 实现 - 创建 OpenGL 资源

#include "OpenGLDevice.h"
#include "OpenGLBuffer.h"
#include "OpenGLShader.h"
#include "OpenGLPipeline.h"
#include "OpenGLCommandBuffer.h"
#include "OpenGLTexture.h"
#include "OpenGLSampler.h"
#include "OpenGLBindGroup.h"
#include "OpenGLRenderTarget.h"
#include "Log/Log.h"
#include <glad/glad.h>

namespace TE {

OpenGLDevice::OpenGLDevice()
{
    TE_LOG_INFO("[RHIOpenGL] OpenGL Device initialized");
    TE_LOG_INFO("[RHIOpenGL] GL_VENDOR: {}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    TE_LOG_INFO("[RHIOpenGL] GL_RENDERER: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    TE_LOG_INFO("[RHIOpenGL] GL_VERSION: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    // 初始化后端特征
    m_Traits.backendType = ERHIBackendType::OpenGL;
    m_Traits.bNativeNDCYAxisUp = true;
    m_Traits.bNativeTextureOriginTopLeft = false;
    m_Traits.bRTSampleRequiresFlipY = false;

    // 当前 OpenGL 路径以 4.5 Core 为目标，优先直接启用 glClipControl，
    // 让引擎内部统一使用 [0,1] 深度约定。
    m_Traits.bSupportsClipControl = GLAD_GL_VERSION_4_5 != 0;
    m_Traits.bNativeNDCDepthZeroToOne = false;

    if (m_Traits.bSupportsClipControl)
    {
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        m_Traits.bNativeNDCDepthZeroToOne = true;
        TE_LOG_INFO("[RHIOpenGL] glClipControl enabled: NDC depth [0, 1]");
    }

    if (!m_Traits.bNativeNDCDepthZeroToOne)
    {
        TE_LOG_INFO("[RHIOpenGL] NDC depth remains [-1, 1], using matrix remapping via AdjustProjectionMatrix");
    }
}

OpenGLDevice::~OpenGLDevice()
{
    TE_LOG_INFO("[RHIOpenGL] OpenGL Device destroyed");
}

std::unique_ptr<RHIBuffer> OpenGLDevice::CreateBuffer(const RHIBufferDesc& desc)
{
    return std::make_unique<OpenGLBuffer>(desc);
}

std::unique_ptr<RHIShader> OpenGLDevice::CreateShader(const RHIShaderDesc& desc)
{
    auto shader = std::make_unique<OpenGLShader>(desc);
    if (!shader->IsValid())
    {
        TE_LOG_ERROR("[RHIOpenGL] Failed to create shader: {}", desc.logicalName);
        return nullptr;
    }
    return shader;
}

std::unique_ptr<RHIPipeline> OpenGLDevice::CreatePipeline(const RHIPipelineDesc& desc)
{
    auto pipeline = std::make_unique<OpenGLPipeline>(desc);
    if (!pipeline->IsValid())
    {
        TE_LOG_ERROR("[RHIOpenGL] Failed to create pipeline");
        return nullptr;
    }
    return pipeline;
}

std::unique_ptr<RHICommandBuffer> OpenGLDevice::CreateCommandBuffer()
{
    return std::make_unique<OpenGLCommandBuffer>();
}

std::unique_ptr<RHITexture> OpenGLDevice::CreateTexture(const RHITextureDesc& desc)
{
    auto texture = std::make_unique<OpenGLTexture>(desc);
    if (!texture->IsValid())
    {
        TE_LOG_ERROR("[RHIOpenGL] Failed to create texture '{}'", desc.debugName);
        return nullptr;
    }
    return texture;
}

std::unique_ptr<RHISampler> OpenGLDevice::CreateSampler(const RHISamplerDesc& desc)
{
    auto sampler = std::make_unique<OpenGLSampler>(desc);
    if (!sampler->IsValid())
    {
        TE_LOG_ERROR("[RHIOpenGL] Failed to create sampler '{}'", desc.debugName);
        return nullptr;
    }
    return sampler;
}

std::unique_ptr<RHIRenderTarget> OpenGLDevice::CreateRenderTarget(const RHIRenderTargetDesc& desc)
{
    auto renderTarget = std::make_unique<OpenGLRenderTarget>(desc);
    if (!renderTarget->IsValid())
    {
        TE_LOG_ERROR("[RHIOpenGL] Failed to create RenderTarget '{}'", desc.debugName);
        return nullptr;
    }
    return renderTarget;
}

std::unique_ptr<RHIBindGroup> OpenGLDevice::CreateBindGroup(const RHIBindGroupDesc& desc)
{
    return std::make_unique<OpenGLBindGroup>(desc);
}

const RHIBackendTraits& OpenGLDevice::GetBackendTraits() const
{
    return m_Traits;
}

Matrix4 OpenGLDevice::AdjustProjectionMatrix(const Matrix4& projection) const
{
    if (m_Traits.bNativeNDCDepthZeroToOne)
    {
        // glClipControl 已开启，引擎约定的 [0,1] 投影矩阵可直接使用
        return projection;
    }

    // 将 [0,1] 深度范围重映射到 OpenGL 原生的 [-1,1]。
    // 重映射矩阵：z_ndc = 2 * z_01 - 1，即对 Z 分量做 scale=2 bias=-1：
    //   | 1  0  0  0 |
    //   | 0  1  0  0 |
    //   | 0  0  2  0 |
    //   | 0  0 -1  1 |
    // 列主序下 M[col][row]，因此 M[2][2]=2, M[3][2]=-1
    Matrix4 remap(1.0f);
    remap(2, 2) = 2.0f;
    remap(3, 2) = -1.0f;

    return remap * projection;
}

} // namespace TE
