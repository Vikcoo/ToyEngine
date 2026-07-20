#version 450 core

#include "../Common/RHIDescriptorBindings.glsl"

layout(location = 0) out vec2 vScreenUV;

void main()
{
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );

    vec2 position = positions[TE_VERTEX_INDEX];
    vScreenUV = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}
