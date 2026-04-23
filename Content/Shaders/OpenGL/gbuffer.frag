#version 410 core

in vec3 vWorldNormal;
in vec3 vWorldPosition;
in vec2 vTexCoord;
in vec3 vColor;

uniform sampler2D u_BaseColorTex;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outWorldPosition;

void main()
{
    vec3 baseColor = texture(u_BaseColorTex, vTexCoord).rgb * vColor;
    vec3 encodedNormal = normalize(vWorldNormal) * 0.5 + 0.5;

    outAlbedo = vec4(baseColor, 1.0);
    outNormal = vec4(encodedNormal, 1.0);
    outWorldPosition = vec4(vWorldPosition, 1.0);
}
