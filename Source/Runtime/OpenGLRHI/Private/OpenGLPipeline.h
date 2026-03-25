// ToyEngine OpenGLRHI Module
// OpenGL Pipeline 实现 - 封装 Program + VAO 配置
// 在 OpenGL 中，Pipeline 对应 Shader Program + 顶点属性布局 + 光栅化状态的组合

#pragma once

#include "RHIPipeline.h"
#include "RHITypes.h"
#include <glad/glad.h>

namespace TE {

class OpenGLShader;

class OpenGLPipeline final : public RHIPipeline
{
public:
    OpenGLPipeline(const RHIPipelineDesc& desc);
    ~OpenGLPipeline() override;

    // 禁止拷贝
    OpenGLPipeline(const OpenGLPipeline&) = delete;
    OpenGLPipeline& operator=(const OpenGLPipeline&) = delete;

    [[nodiscard]] bool IsValid() const override { return m_ProgramID != 0 && m_VAO != 0; }

    /// 获取 OpenGL Program ID
    [[nodiscard]] GLuint GetGLProgram() const { return m_ProgramID; }

    /// 获取 VAO（存储了顶点属性布局）
    [[nodiscard]] GLuint GetVAO() const { return m_VAO; }

    /// 获取图元拓扑对应的 GL 枚举
    [[nodiscard]] GLenum GetGLPrimitiveTopology() const { return m_GLTopology; }

    /// 获取管线描述信息（用于 CommandBuffer 设置状态）
    [[nodiscard]] const RHIRasterizationDesc& GetRasterizationDesc() const { return m_Rasterization; }
    [[nodiscard]] const RHIDepthStencilDesc& GetDepthStencilDesc() const { return m_DepthStencil; }

    /// 获取顶点输入布局描述（用于 BindVertexBuffer 时配置顶点属性）
    [[nodiscard]] const RHIVertexInputDesc& GetVertexInputDesc() const { return m_VertexInput; }

private:
    /// 创建并链接 Program
    [[nodiscard]] bool LinkProgram(GLuint vertShader, GLuint fragShader);

    /// 配置 VAO 顶点属性（仅 enable + 记录 format，实际关联 VBO 在 BindVertexBuffer 时）
    void SetupVertexAttributes(const RHIVertexInputDesc& vertexInput);

    GLuint                  m_ProgramID = 0;
    GLuint                  m_VAO = 0;
    GLenum                  m_GLTopology = GL_TRIANGLES;
    RHIRasterizationDesc    m_Rasterization;
    RHIDepthStencilDesc     m_DepthStencil;
    RHIVertexInputDesc      m_VertexInput;
};

} // namespace TE
