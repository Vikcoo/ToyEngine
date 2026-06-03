#version 450 core

const int MaxDirectionalLights = 4;
const int MaxPointLights = 8;

in vec2 vScreenUV;

uniform sampler2D u_GBufferAlbedo;
uniform sampler2D u_GBufferNormal;
uniform sampler2D u_GBufferWorldPosition;
uniform sampler2D u_GBufferDepth;
uniform int u_RTSampleFlipY;
uniform int u_DebugViewMode;

layout(std140, binding = 0) uniform LightBlock {
    ivec4 u_LightCounts;
    vec4 u_DirectionalLightDirections[MaxDirectionalLights];
    vec4 u_DirectionalLightColors[MaxDirectionalLights];
    vec4 u_PointLightPositions[MaxPointLights];
    vec4 u_PointLightColorsAndRadii[MaxPointLights];
};

out vec4 fragColor;

void main()
{
    vec2 uv = vScreenUV;
    if (u_RTSampleFlipY != 0)
    {
        uv.y = 1.0 - uv.y;
    }

    float depth = texture(u_GBufferDepth, uv).r;
    if (depth >= 0.999999)
    {
        fragColor = vec4(0.1, 0.1, 0.1, 1.0);
        return;
    }

    vec3 baseColor = texture(u_GBufferAlbedo, uv).rgb;
    vec3 normal = normalize(texture(u_GBufferNormal, uv).rgb * 2.0 - 1.0);
    vec3 worldPosition = texture(u_GBufferWorldPosition, uv).rgb;

    if (u_DebugViewMode == 1)
    {
        fragColor = vec4(baseColor, 1.0);
        return;
    }
    if (u_DebugViewMode == 2)
    {
        fragColor = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    }
    if (u_DebugViewMode == 3)
    {
        vec3 worldPositionView = clamp(worldPosition * 0.1 + 0.5, 0.0, 1.0);
        fragColor = vec4(worldPositionView, 1.0);
        return;
    }
    if (u_DebugViewMode == 4)
    {
        fragColor = vec4(vec3(depth), 1.0);
        return;
    }

    vec3 lighting = vec3(0.08);

    for (int i = 0; i < u_LightCounts.x; ++i)
    {
        vec3 lightDir = normalize(u_DirectionalLightDirections[i].xyz);
        float nDotL = max(dot(normal, lightDir), 0.0);
        lighting += nDotL * u_DirectionalLightColors[i].xyz;
    }

    for (int i = 0; i < u_LightCounts.y; ++i)
    {
        vec3 lightVector = u_PointLightPositions[i].xyz - worldPosition;
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
