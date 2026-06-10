// ToyEngine RHIOpenGL Module
// OpenGL PipelineLayout 实现

#pragma once

#include "RHIPipeline.h"
#include "RHITypes.h"

namespace TE {

class OpenGLPipelineLayout final : public RHIPipelineLayout
{
public:
    explicit OpenGLPipelineLayout(const RHIPipelineLayoutDesc& desc);
    ~OpenGLPipelineLayout() override = default;

    [[nodiscard]] bool IsValid() const override { return m_Valid; }
    [[nodiscard]] const RHIPipelineLayoutDesc& GetDesc() const { return m_Desc; }

private:
    RHIPipelineLayoutDesc m_Desc;
    bool m_Valid = false;
};

} // namespace TE
