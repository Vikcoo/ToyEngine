// ToyEngine OpenGLRHI Module
// OpenGL Shader 实现 - 封装 glCreateShader / glCompileShader
// 支持从文件路径读取 GLSL 源码

#pragma once

#include "RHIShader.h"
#include <glad/glad.h>

namespace TE {

class OpenGLShader final : public RHIShader
{
public:
    OpenGLShader(const RHIShaderDesc& desc);
    ~OpenGLShader() override;

    // 禁止拷贝
    OpenGLShader(const OpenGLShader&) = delete;
    OpenGLShader& operator=(const OpenGLShader&) = delete;

    [[nodiscard]] RHIShaderStage GetStage() const override { return m_Stage; }
    [[nodiscard]] bool IsValid() const override { return m_ShaderID != 0 && m_Compiled; }

    /// 获取 OpenGL Shader ID（用于 glAttachShader）
    [[nodiscard]] GLuint GetGLShaderID() const { return m_ShaderID; }

private:
    /// 从文件读取着色器源码
    [[nodiscard]] static bool ReadShaderFile(const std::string& filePath, std::string& outSource);

    /// 编译着色器
    [[nodiscard]] bool Compile(const std::string& source);

    GLuint          m_ShaderID = 0;
    RHIShaderStage  m_Stage = RHIShaderStage::Vertex;
    bool            m_Compiled = false;
};

} // namespace TE
