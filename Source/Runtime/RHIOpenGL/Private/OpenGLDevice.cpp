// ToyEngine RHIOpenGL Module
// OpenGL Device 实现 - 创建 OpenGL 资源

#include "OpenGLDevice.h"
#include "OpenGLBuffer.h"
#include "OpenGLShader.h"
#include "OpenGLPipeline.h"
#include "OpenGLCommandBuffer.h"
#include "OpenGLTexture.h"
#include "OpenGLSampler.h"
#include "Log/Log.h"
#include <glad/glad.h>

namespace TE {

OpenGLDevice::OpenGLDevice()
{
    TE_LOG_INFO("[RHIOpenGL] OpenGL Device initialized");
    TE_LOG_INFO("[RHIOpenGL] GL_VENDOR: {}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    TE_LOG_INFO("[RHIOpenGL] GL_RENDERER: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    TE_LOG_INFO("[RHIOpenGL] GL_VERSION: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
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

} // namespace TE
