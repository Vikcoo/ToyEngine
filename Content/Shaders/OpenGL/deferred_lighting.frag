#version 450 core

const int MaxDirectionalLights = 4;
const int MaxPointLights = 8;
const float PI = 3.14159265359;

in vec2 vScreenUV;

layout(binding = 2) uniform sampler2D u_GBufferAlbedo;
layout(binding = 3) uniform sampler2D u_GBufferNormal;
layout(binding = 4) uniform sampler2D u_GBufferWorldPosition;
layout(binding = 5) uniform sampler2D u_GBufferDepth;
layout(binding = 6) uniform sampler2D u_GBufferMaterial;
layout(binding = 9) uniform samplerCube u_IrradianceMap;
layout(binding = 10) uniform samplerCube u_PrefilterMap;
layout(binding = 11) uniform sampler2D u_BRDFLUT;

layout(std140, binding = 1) uniform DeferredPassBlock {
    ivec4 u_DeferredParams;
    vec4 u_CameraPosition_Pad;
    mat4 u_InvViewProjection;
};

layout(std140, binding = 0) uniform LightBlock {
    ivec4 u_LightCounts;
    vec4 u_DirectionalLightDirections[MaxDirectionalLights];
    vec4 u_DirectionalLightColors[MaxDirectionalLights];
    vec4 u_PointLightPositions[MaxPointLights];
    vec4 u_PointLightColorsAndRadii[MaxPointLights];
};

out vec4 fragColor;

vec3 TonemapReinhard(vec3 color)
{
    return color / (color + vec3(1.0));
}

vec3 GetSkyColor(vec2 uv)
{
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 world = u_InvViewProjection * vec4(ndc, 1.0, 1.0);
    world.xyz /= max(world.w, 0.0001);
    vec3 dir = normalize(world.xyz - u_CameraPosition_Pad.xyz);
    return TonemapReinhard(textureLod(u_PrefilterMap, dir, 0.0).rgb);
}

float DistributionGGX(vec3 n, vec3 h, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float nDotH = max(dot(n, h), 0.0);
    float nDotH2 = nDotH * nDotH;
    float denom = (nDotH2 * (a2 - 1.0) + 1.0);
    return a2 / max(PI * denom * denom, 0.0001);
}

float GeometrySchlickGGX(float nDotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return nDotV / max(nDotV * (1.0 - k) + k, 0.0001);
}

float GeometrySmith(vec3 n, vec3 v, vec3 l, float roughness)
{
    return GeometrySchlickGGX(max(dot(n, v), 0.0), roughness) *
           GeometrySchlickGGX(max(dot(n, l), 0.0), roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 f0, float roughness)
{
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 EvaluatePBRLight(vec3 n, vec3 v, vec3 l, vec3 radiance, vec3 baseColor, float metallic, float roughness)
{
    vec3 h = normalize(v + l);
    float nDotL = max(dot(n, l), 0.0);
    float nDotV = max(dot(n, v), 0.0);
    if (nDotL <= 0.0 || nDotV <= 0.0)
    {
        return vec3(0.0);
    }

    vec3 f0 = mix(vec3(0.04), baseColor, metallic);
    float ndf = DistributionGGX(n, h, roughness);
    float g = GeometrySmith(n, v, l, roughness);
    vec3 f = FresnelSchlick(max(dot(h, v), 0.0), f0);

    vec3 specular = (ndf * g * f) / max(4.0 * nDotV * nDotL, 0.0001);
    vec3 kS = f;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    return (kD * baseColor / PI + specular) * radiance * nDotL;
}

void main()
{
    vec2 uv = vScreenUV;
    if (u_DeferredParams.x != 0)
    {
        uv.y = 1.0 - uv.y;
    }

    float depth = texture(u_GBufferDepth, uv).r;
    if (depth >= 0.999999)
    {
        fragColor = vec4(GetSkyColor(uv), 1.0);
        return;
    }

    vec3 baseColor = texture(u_GBufferAlbedo, uv).rgb;
    vec3 normal = normalize(texture(u_GBufferNormal, uv).rgb * 2.0 - 1.0);
    vec3 worldPosition = texture(u_GBufferWorldPosition, uv).rgb;
    vec4 material = texture(u_GBufferMaterial, uv);
    float metallic = material.r;
    float roughness = clamp(material.g, 0.04, 1.0);
    float ao = material.b;
    float emissive = material.a;

    if (u_DeferredParams.y == 1)
    {
        fragColor = vec4(baseColor, 1.0);
        return;
    }
    if (u_DeferredParams.y == 2)
    {
        fragColor = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    }
    if (u_DeferredParams.y == 3)
    {
        vec3 worldPositionView = clamp(worldPosition * 0.1 + 0.5, 0.0, 1.0);
        fragColor = vec4(worldPositionView, 1.0);
        return;
    }
    if (u_DeferredParams.y == 4)
    {
        fragColor = vec4(vec3(depth), 1.0);
        return;
    }

    vec3 v = normalize(u_CameraPosition_Pad.xyz - worldPosition);
    vec3 f0 = mix(vec3(0.04), baseColor, metallic);
    float nDotV = max(dot(normal, v), 0.0);
    vec3 f = FresnelSchlickRoughness(nDotV, f0, roughness);
    vec3 kS = f;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    vec3 irradiance = texture(u_IrradianceMap, normal).rgb;
    vec3 diffuseIBL = irradiance * baseColor;
    vec3 reflection = reflect(-v, normal);
    vec3 prefilteredColor = textureLod(u_PrefilterMap, reflection, roughness * 4.0).rgb;
    vec2 brdf = texture(u_BRDFLUT, vec2(nDotV, roughness)).rg;
    vec3 specularIBL = prefilteredColor * (f * brdf.x + brdf.y);
    vec3 color = (kD * diffuseIBL + specularIBL) * ao;

    for (int i = 0; i < u_LightCounts.x; ++i)
    {
        vec3 l = normalize(u_DirectionalLightDirections[i].xyz);
        color += EvaluatePBRLight(normal, v, l, u_DirectionalLightColors[i].xyz, baseColor, metallic, roughness);
    }

    for (int i = 0; i < u_LightCounts.y; ++i)
    {
        vec3 lightVector = u_PointLightPositions[i].xyz - worldPosition;
        float distanceToLight = length(lightVector);
        float radius = max(u_PointLightColorsAndRadii[i].w, 0.001);
        vec3 l = lightVector / max(distanceToLight, 0.001);
        float attenuation = clamp(1.0 - distanceToLight / radius, 0.0, 1.0);
        attenuation *= attenuation;
        vec3 radiance = u_PointLightColorsAndRadii[i].xyz * attenuation;
        color += EvaluatePBRLight(normal, v, l, radiance, baseColor, metallic, roughness);
    }

    fragColor = vec4(color + vec3(emissive), 1.0);
}
