// ToyEngine RHIOpenGL Module
// OpenGL Pipeline 实现 - Program 链接 + VAO 顶点属性配置

#include "OpenGLPipeline.h"
#include "OpenGLShader.h"
#include "Log/Log.h"

namespace TE {

static GLenum TopologyToGL(RHIPrimitiveTopology topology)
{
    switch (topology)
    {
        case RHIPrimitiveTopology::PointList:       return GL_POINTS;
        case RHIPrimitiveTopology::LineList:         return GL_LINES;
        case RHIPrimitiveTopology::LineStrip:        return GL_LINE_STRIP;
        case RHIPrimitiveTopology::TriangleList:     return GL_TRIANGLES;
        case RHIPrimitiveTopology::TriangleStrip:    return GL_TRIANGLE_STRIP;
        default:                                     return GL_TRIANGLES;
    }
}

static GLenum FormatToGLType(RHIFormat format)
{
    switch (format)
    {
        case RHIFormat::Float:
        case RHIFormat::Float2:
        case RHIFormat::Float3:
        case RHIFormat::Float4:
            return GL_FLOAT;
        case RHIFormat::Int:
        case RHIFormat::Int2:
        case RHIFormat::Int3:
        case RHIFormat::Int4:
            return GL_INT;
        case RHIFormat::UInt:
        case RHIFormat::UInt2:
        case RHIFormat::UInt3:
        case RHIFormat::UInt4:
            return GL_UNSIGNED_INT;
        default:
            return GL_FLOAT;
    }
}

static GLint FormatToComponentCount(RHIFormat format)
{
    switch (format)
    {
        case RHIFormat::Float:  case RHIFormat::Int:  case RHIFormat::UInt:  return 1;
        case RHIFormat::Float2: case RHIFormat::Int2: case RHIFormat::UInt2: return 2;
        case RHIFormat::Float3: case RHIFormat::Int3: case RHIFormat::UInt3: return 3;
        case RHIFormat::Float4: case RHIFormat::Int4: case RHIFormat::UInt4: return 4;
        default: return 0;
    }
}

OpenGLPipeline::OpenGLPipeline(const RHIPipelineDesc& desc)
    : m_GLTopology(TopologyToGL(desc.topology))
    , m_Rasterization(desc.rasterization)
    , m_DepthStencil(desc.depthStencil)
    , m_VertexInput(desc.vertexInput)
{
    // 获取 OpenGL Shader ID
    auto* vertShader = static_cast<OpenGLShader*>(desc.vertexShader);
    auto* fragShader = static_cast<OpenGLShader*>(desc.fragmentShader);

    if (!vertShader || !vertShader->IsValid())
    {
        TE_LOG_ERROR("[RHIOpenGL] Pipeline creation failed: invalid vertex shader");
        return;
    }

    if (!fragShader || !fragShader->IsValid())
    {
        TE_LOG_ERROR("[RHIOpenGL] Pipeline creation failed: invalid fragment shader");
        return;
    }

    // 链接 Program
    if (!LinkProgram(vertShader->GetGLShaderID(), fragShader->GetGLShaderID()))
    {
        return;
    }

    // 创建 VAO 并配置顶点属性
    glGenVertexArrays(1, &m_VAO);
    SetupVertexAttributes(desc.vertexInput);

    if (!desc.debugName.empty())
    {
        TE_LOG_DEBUG("[RHIOpenGL] Pipeline '{}' created: Program={}, VAO={}", desc.debugName, m_ProgramID, m_VAO);
    }
}

OpenGLPipeline::~OpenGLPipeline()
{
    if (m_VAO != 0)
    {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }

    if (m_ProgramID != 0)
    {
        glDeleteProgram(m_ProgramID);
        m_ProgramID = 0;
    }
}

bool OpenGLPipeline::LinkProgram(GLuint vertShader, GLuint fragShader)
{
    m_ProgramID = glCreateProgram();
    glAttachShader(m_ProgramID, vertShader);
    glAttachShader(m_ProgramID, fragShader);
    glLinkProgram(m_ProgramID);

    // 检查链接结果
    GLint success = 0;
    glGetProgramiv(m_ProgramID, GL_LINK_STATUS, &success);

    if (!success)
    {
        GLint logLength = 0;
        glGetProgramiv(m_ProgramID, GL_INFO_LOG_LENGTH, &logLength);

        std::string infoLog(static_cast<size_t>(logLength), '\0');
        glGetProgramInfoLog(m_ProgramID, logLength, nullptr, infoLog.data());

        TE_LOG_ERROR("[RHIOpenGL] Program link failed:\n{}", infoLog);

        glDeleteProgram(m_ProgramID);
        m_ProgramID = 0;
        return false;
    }

    return true;
}

void OpenGLPipeline::SetupVertexAttributes(const RHIVertexInputDesc& vertexInput)
{
    glBindVertexArray(m_VAO);

    if (vertexInput.attributes.empty())
    {
        glBindVertexArray(0);
        return;
    }

    if (vertexInput.bindings.empty())
    {
        TE_LOG_WARN("[RHIOpenGL] Pipeline has vertex attributes but no vertex bindings");
        glBindVertexArray(0);
        return;
    }

    // 配置每个顶点属性
    for (const auto& attr : vertexInput.attributes)
    {
        // 查找对应 binding 的 stride
        uint32_t stride = 0;
        for (const auto& binding : vertexInput.bindings)
        {
            if (binding.binding == 0) // 当前简化，仅支持单个 binding
            {
                stride = binding.stride;
                break;
            }
        }

        glEnableVertexAttribArray(attr.location);
        glVertexAttribPointer(
            attr.location,
            FormatToComponentCount(attr.format),
            FormatToGLType(attr.format),
            GL_FALSE,
            static_cast<GLsizei>(stride),
            reinterpret_cast<const void*>(static_cast<uintptr_t>(attr.offset))
        );
    }

    glBindVertexArray(0);
}

} // namespace TE
