#version 450 core

const int MaxDirectionalLights = 4;
const int MaxPointLights = 8;

in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec2 vTexCoord;
in vec3 vColor;

layout(std140, binding = 0) uniform LightBlock {
    ivec4 u_LightCounts;
    vec4 u_DirectionalLightDirections[MaxDirectionalLights];
    vec4 u_DirectionalLightColors[MaxDirectionalLights];
    vec4 u_PointLightPositions[MaxPointLights];
    vec4 u_PointLightColorsAndRadii[MaxPointLights];
};

layout(binding = 2) uniform sampler2D u_BaseColorTex;

out vec4 fragColor;

void main()
{
    vec3 normal = normalize(vWorldNormal);
    vec3 baseColor = texture(u_BaseColorTex, vTexCoord).rgb * vColor;
    vec3 lighting = vec3(0.08);

    for (int i = 0; i < u_LightCounts.x; ++i)
    {
        vec3 lightDir = normalize(u_DirectionalLightDirections[i].xyz);
        float nDotL = max(dot(normal, lightDir), 0.0);
        lighting += nDotL * u_DirectionalLightColors[i].xyz;
    }

    for (int i = 0; i < u_LightCounts.y; ++i)
    {
        vec3 lightVector = u_PointLightPositions[i].xyz - vWorldPosition;
        float distanceToLight = length(lightVector);
        float radius = max(u_PointLightColorsAndRadii[i].w, 0.001);
        vec3 lightDir = lightVector / max(distanceToLight, 0.001);
        float attenuation = clamp(1.0 - distanceToLight / radius, 0.0, 1.0);
        attenuation *= attenuation;

        float nDotL = max(dot(normal, lightDir), 0.0);
        lighting += nDotL * attenuation * u_PointLightColorsAndRadii[i].xyz;
    }

    fragColor = vec4(baseColor * lighting, 1.0);
}
