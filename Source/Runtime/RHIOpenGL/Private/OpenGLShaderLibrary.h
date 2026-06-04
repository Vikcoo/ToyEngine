// ToyEngine RHIOpenGL Module
// OpenGLShaderLibrary - 将逻辑 Shader 名称解析为 OpenGL GLSL 资产

#pragma once

#include <string>

namespace TE {

[[nodiscard]] std::string ResolveOpenGLShaderPath(const std::string& logicalName);

} // namespace TE
