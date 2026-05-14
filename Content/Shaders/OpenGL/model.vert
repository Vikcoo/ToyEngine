#version 410 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;

uniform mat4 u_MVP;
uniform mat4 u_Model;
uniform mat3 u_NormalMatrix;

out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec2 vTexCoord;
out vec3 vColor;

void main()
{
    vec4 worldPosition = u_Model * vec4(aPosition, 1.0);
    gl_Position = u_MVP * vec4(aPosition, 1.0);
    vWorldPosition = worldPosition.xyz;
    vWorldNormal = normalize(u_NormalMatrix * aNormal);
    vTexCoord = aTexCoord;
    vColor = aColor;
}
