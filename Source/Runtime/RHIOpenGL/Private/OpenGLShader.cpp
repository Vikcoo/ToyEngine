// ToyEngine RHIOpenGL Module
// OpenGL Shader 实现 - GLSL 编译

#include "OpenGLShader.h"
#include "OpenGLShaderLibrary.h"
#include "Log/Log.h"
#include <filesystem>
#include <fstream>
#include <string_view>
#include <unordered_set>

namespace TE {

namespace {

bool ExpandShaderFile(const std::filesystem::path& filePath,
                      std::string& outSource,
                      std::unordered_set<std::string>& activeIncludes)
{
    const std::filesystem::path normalizedPath = filePath.lexically_normal();
    const std::string includeKey = normalizedPath.generic_string();
    if (!activeIncludes.insert(includeKey).second)
    {
        TE_LOG_ERROR("[RHIOpenGL] Cyclic shader include detected: {}", includeKey);
        return false;
    }

    std::ifstream file(normalizedPath, std::ios::in);
    if (!file.is_open())
    {
        TE_LOG_ERROR("[RHIOpenGL] Cannot open shader file: {}", includeKey);
        activeIncludes.erase(includeKey);
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        const std::string_view lineView(line);
        const size_t firstNonWhitespace = lineView.find_first_not_of(" \t");
        const std::string_view trimmed = firstNonWhitespace == std::string_view::npos
            ? std::string_view{}
            : lineView.substr(firstNonWhitespace);

        if (trimmed.starts_with("#include"))
        {
            const size_t firstQuote = trimmed.find('"');
            const size_t secondQuote = firstQuote == std::string_view::npos
                ? std::string_view::npos
                : trimmed.find('"', firstQuote + 1);
            if (firstQuote == std::string_view::npos || secondQuote == std::string_view::npos)
            {
                TE_LOG_ERROR("[RHIOpenGL] Invalid shader include in '{}': {}", includeKey, line);
                activeIncludes.erase(includeKey);
                return false;
            }

            const std::filesystem::path includePath = normalizedPath.parent_path() /
                std::string(trimmed.substr(firstQuote + 1, secondQuote - firstQuote - 1));
            if (!ExpandShaderFile(includePath, outSource, activeIncludes))
            {
                activeIncludes.erase(includeKey);
                return false;
            }
            continue;
        }

        outSource.append(line);
        outSource.push_back('\n');
    }

    activeIncludes.erase(includeKey);
    return true;
}

} // namespace

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
    const std::string filePath = ResolveOpenGLShaderPath(desc.logicalName);
    if (filePath.empty())
    {
        return;
    }

    // 读取着色器源码
    std::string source;
    if (!ReadShaderFile(filePath, source))
    {
        TE_LOG_ERROR("[RHIOpenGL] Failed to read shader '{}' from '{}'", desc.logicalName, filePath);
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
    outSource.clear();
    std::unordered_set<std::string> activeIncludes;
    if (!ExpandShaderFile(std::filesystem::path(filePath), outSource, activeIncludes))
    {
        return false;
    }

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
