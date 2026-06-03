#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;

layout(std140, binding = 1) uniform ObjectBlock {
    mat4 u_MVP;
    mat4 u_Model;
    mat4 u_NormalMatrix;
};

out vec3 vWorldNormal;
out vec3 vWorldPosition;
out vec2 vTexCoord;
out vec3 vColor;

void main()
{
    vec4 worldPosition = u_Model * vec4(aPosition, 1.0);
    gl_Position = u_MVP * vec4(aPosition, 1.0);
    vWorldPosition = worldPosition.xyz;
    vWorldNormal = normalize((u_NormalMatrix * vec4(aNormal, 0.0)).xyz);
    vTexCoord = aTexCoord;
    vColor = aColor;
}
