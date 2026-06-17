#version 450 core

in vec2 vScreenUV;

layout(std140, binding = 1) uniform SkyBlock {
    mat4 u_InvViewProjection;
    vec4 u_CameraPosition_Pad;
};

layout(binding = 10) uniform samplerCube u_PrefilterMap;

out vec4 fragColor;

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
