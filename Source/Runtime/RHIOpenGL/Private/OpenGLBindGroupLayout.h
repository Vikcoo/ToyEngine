// ToyEngine RHIOpenGL Module
// OpenGL BindGroupLayout 实现

#pragma once

#include "RHIBindGroup.h"
#include "RHITypes.h"

namespace TE {

class OpenGLBindGroupLayout final : public RHIBindGroupLayout
{
public:
    explicit OpenGLBindGroupLayout(const RHIBindGroupLayoutDesc& desc);
    ~OpenGLBindGroupLayout() override = default;

    [[nodiscard]] bool IsValid() const override { return m_Valid; }
    [[nodiscard]] const RHIBindGroupLayoutDesc& GetDesc() const { return m_Desc; }

private:
    RHIBindGroupLayoutDesc m_Desc;
    bool m_Valid = false;
};

} // namespace TE
