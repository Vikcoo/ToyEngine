// ToyEngine RHIOpenGL Module
// OpenGL Shader 实现 - GLSL 编译

#include "OpenGLShader.h"
#include "Log/Log.h"
#include <fstream>
#include <sstream>

namespace TE {

static GLenum ShaderStageToGL(RHIShaderStage stage)
{
    switch (stage)
    {
        case RHIShaderStage::Vertex:    return GL_VERTEX_SHADER;
        case RHIShaderStage::Fragment:  return GL_FRAGMENT_SHADER;
        case RHIShaderStage::Geometry:  return GL_GEOMETRY_SHADER;
        default:
            TE_LOG_ERROR("[RHIOpenGL] Unsupported shader stage");
            return GL_VERTEX_SHADER;
    }
}

OpenGLShader::OpenGLShader(const RHIShaderDesc& desc)
    : m_Stage(desc.stage)
{
    // 读取着色器源码
    std::string source;
    if (!ReadShaderFile(desc.filePath, source))
    {
        TE_LOG_ERROR("[RHIOpenGL] Failed to read shader file: {}", desc.filePath);
        return;
    }

    // 创建 OpenGL Shader 对象
    m_ShaderID = glCreateShader(ShaderStageToGL(desc.stage));
    if (m_ShaderID == 0)
    {
        TE_LOG_ERROR("[RHIOpenGL] Failed to create shader object");
        return;
    }

    // 编译
    if (!Compile(source))
    {
        glDeleteShader(m_ShaderID);
        m_ShaderID = 0;
        return;
    }

    m_Compiled = true;

    if (!desc.debugName.empty())
    {
        TE_LOG_DEBUG("[RHIOpenGL] Shader '{}' compiled successfully (ID={})", desc.debugName, m_ShaderID);
    }
}

OpenGLShader::~OpenGLShader()
{
    if (m_ShaderID != 0)
    {
        glDeleteShader(m_ShaderID);
        m_ShaderID = 0;
    }
}

bool OpenGLShader::ReadShaderFile(const std::string& filePath, std::string& outSource)
{
    std::ifstream file(filePath, std::ios::in);
    if (!file.is_open())
    {
        TE_LOG_ERROR("[RHIOpenGL] Cannot open shader file: {}", filePath);
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    outSource = ss.str();

    if (outSource.empty())
    {
        TE_LOG_WARN("[RHIOpenGL] Shader file is empty: {}", filePath);
        return false;
    }

    return true;
}

bool OpenGLShader::Compile(const std::string& source)
{
    const char* src = source.c_str();
    glShaderSource(m_ShaderID, 1, &src, nullptr);
    glCompileShader(m_ShaderID);

    // 检查编译结果
    GLint success = 0;
    glGetShaderiv(m_ShaderID, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        GLint logLength = 0;
        glGetShaderiv(m_ShaderID, GL_INFO_LOG_LENGTH, &logLength);

        std::string infoLog(static_cast<size_t>(logLength), '\0');
        glGetShaderInfoLog(m_ShaderID, logLength, nullptr, infoLog.data());

        TE_LOG_ERROR("[RHIOpenGL] Shader compilation failed:\n{}", infoLog);
        return false;
    }

    return true;
}

} // namespace TE
