#version 450

#include "Common/RHIDescriptorBindings.glsl"

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec3 a_Color;

TE_UNIFORM_BINDING(0, 0) uniform StageBObjectBlock
{
    mat4 u_MVP;
};

layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out vec3 v_Color;

void main()
{
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
