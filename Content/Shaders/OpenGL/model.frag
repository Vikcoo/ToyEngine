#version 410 core

const int MaxDirectionalLights = 4;
const int MaxPointLights = 8;

in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec2 vTexCoord;
in vec3 vColor;

uniform int u_DirectionalLightCount;
uniform vec3 u_DirectionalLightDirections[MaxDirectionalLights];
uniform vec3 u_DirectionalLightColors[MaxDirectionalLights];

uniform int u_PointLightCount;
uniform vec3 u_PointLightPositions[MaxPointLights];
uniform vec3 u_PointLightColors[MaxPointLights];
uniform float u_PointLightRadii[MaxPointLights];

uniform sampler2D u_BaseColorTex;

out vec4 fragColor;

void main()
{
    vec3 normal = normalize(vWorldNormal);
    vec3 baseColor = texture(u_BaseColorTex, vTexCoord).rgb * vColor;
    vec3 lighting = vec3(0.08);

    for (int i = 0; i < u_DirectionalLightCount; ++i)
    {
        vec3 lightDir = normalize(u_DirectionalLightDirections[i]);
        float nDotL = max(dot(normal, lightDir), 0.0);
        lighting += nDotL * u_DirectionalLightColors[i];
    }

    for (int i = 0; i < u_PointLightCount; ++i)
    {
        vec3 lightVector = u_PointLightPositions[i] - vWorldPosition;
        float distanceToLight = length(lightVector);
        float radius = max(u_PointLightRadii[i], 0.001);
        vec3 lightDir = lightVector / max(distanceToLight, 0.001);
        float attenuation = clamp(1.0 - distanceToLight / radius, 0.0, 1.0);
        attenuation *= attenuation;

        float nDotL = max(dot(normal, lightDir), 0.0);
        lighting += nDotL * attenuation * u_PointLightColors[i];
    }

    fragColor = vec4(baseColor * lighting, 1.0);
}
