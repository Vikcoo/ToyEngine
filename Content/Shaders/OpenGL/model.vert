#version 450 core

#include "../Common/RHIDescriptorBindings.glsl"

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;
layout(location = 4) in vec3 aTangent;

TE_UNIFORM_BINDING(1, 1) uniform ObjectBlock {
    mat4 u_MVP;
    mat4 u_Model;
    mat4 u_NormalMatrix;
};

layout(location = 0) out vec3 vWorldPosition;
layout(location = 1) out vec3 vWorldNormal;
layout(location = 2) out vec3 vWorldTangent;
layout(location = 3) out vec2 vTexCoord;
layout(location = 4) out vec3 vColor;

void main()
{
    vec4 worldPosition = u_Model * vec4(aPosition, 1.0);
    gl_Position = u_MVP * vec4(aPosition, 1.0);
    vWorldPosition = worldPosition.xyz;
    vWorldNormal = normalize((u_NormalMatrix * vec4(aNormal, 0.0)).xyz);
    vWorldTangent = normalize((u_NormalMatrix * vec4(aTangent, 0.0)).xyz);
    vTexCoord = aTexCoord;
    vColor = aColor;
}
