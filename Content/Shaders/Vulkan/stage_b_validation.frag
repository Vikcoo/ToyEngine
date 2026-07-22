#version 450

#include "Common/RHIDescriptorBindings.glsl"

TE_RESOURCE_BINDING(1, 0) uniform sampler2D u_Texture;

layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in vec3 v_Color;
layout(location = 0) out vec4 o_Color;

void main()
{
    o_Color = texture(u_Texture, v_TexCoord) * vec4(v_Color, 1.0);
}
