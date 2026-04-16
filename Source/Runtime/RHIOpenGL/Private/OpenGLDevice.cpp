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
#include <string>
#ifdef TE_PLATFORM_WINDOWS
#include <Windows.h>
#endif

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

    // 检测 GL_ARB_clip_control（OpenGL 4.5+ 核心功能，或作为扩展出现）。
    // 当前项目目标为 OpenGL 4.1 Core（macOS 兼容），glad 配置中不包含此扩展。
    // 通过运行时枚举扩展检测是否可用。
    m_Traits.bSupportsClipControl = false;
    m_Traits.bNativeNDCDepthZeroToOne = false;

    GLint numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    for (GLint i = 0; i < numExtensions; ++i)
    {
        const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        if (ext && std::string(ext) == "GL_ARB_clip_control")
        {
            m_Traits.bSupportsClipControl = true;
            break;
        }
    }

    if (m_Traits.bSupportsClipControl)
    {
        // 动态加载 glClipControl（glad 4.1 配置中无此符号）
        using PFN_glClipControl = void(APIENTRYP)(GLenum, GLenum);
        auto pfn = reinterpret_cast<PFN_glClipControl>(gladLoadGL());
        // gladLoadGL 不适用于单个函数，改为直接使用 glad 内部机制
        // 这里用平台相关方式获取函数指针
#ifdef TE_PLATFORM_WINDOWS
        auto fnClipControl = reinterpret_cast<void(APIENTRYP)(GLenum, GLenum)>(
            wglGetProcAddress("glClipControl"));
#else
        void(*fnClipControl)(GLenum, GLenum) = nullptr;
#endif
        if (fnClipControl)
        {
            constexpr GLenum GL_ZERO_TO_ONE_ARB = 0x935F;
            fnClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE_ARB);
            m_Traits.bNativeNDCDepthZeroToOne = true;
            TE_LOG_INFO("[RHIOpenGL] glClipControl enabled: NDC depth [0, 1]");
        }
        else
        {
            m_Traits.bSupportsClipControl = false;
        }
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
        TE_LOG_ERROR("[RHIOpenGL] Failed to create shader: {}", desc.filePath);
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
