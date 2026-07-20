#version 450 core

#include "../Common/RHIDescriptorBindings.glsl"

layout(location = 0) in vec2 vScreenUV;

TE_UNIFORM_BINDING(1, 1) uniform SkyBlock {
    mat4 u_InvViewProjection;
    vec4 u_CameraPosition_Pad;
};

TE_RESOURCE_BINDING(4, 10) uniform samplerCube u_PrefilterMap;

layout(location = 0) out vec4 fragColor;

vec3 TonemapReinhard(vec3 color)
{
    return color / (color + vec3(1.0));
}

void main()
{
    vec2 ndc = vScreenUV * 2.0 - 1.0;
    vec4 world = u_InvViewProjection * vec4(ndc, 1.0, 1.0);
    world.xyz /= max(world.w, 0.0001);
    vec3 dir = normalize(world.xyz - u_CameraPosition_Pad.xyz);
    vec3 sky = textureLod(u_PrefilterMap, dir, 0.0).rgb;
    fragColor = vec4(TonemapReinhard(sky), 1.0);
}
