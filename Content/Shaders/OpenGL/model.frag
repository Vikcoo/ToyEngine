#version 450 core

const int MaxDirectionalLights = 4;
const int MaxPointLights = 8;
const float PI = 3.14159265359;

in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec3 vWorldTangent;
in vec2 vTexCoord;
in vec3 vColor;

layout(std140, binding = 0) uniform LightBlock {
    ivec4 u_LightCounts;
    vec4 u_DirectionalLightDirections[MaxDirectionalLights];
    vec4 u_DirectionalLightColors[MaxDirectionalLights];
    vec4 u_PointLightPositions[MaxPointLights];
    vec4 u_PointLightColorsAndRadii[MaxPointLights];
};

layout(std140, binding = 8) uniform MaterialBlock {
    vec4 u_BaseColorFactor_Metallic;
    vec4 u_RoughnessAOEmissiveStrength_Pad;
    vec4 u_EmissiveFactor_Pad;
    vec4 u_CameraPosition_Pad;
};

layout(binding = 2) uniform sampler2D u_BaseColorTex;
layout(binding = 3) uniform sampler2D u_NormalTex;
layout(binding = 4) uniform sampler2D u_MetallicTex;
layout(binding = 5) uniform sampler2D u_RoughnessTex;
layout(binding = 6) uniform sampler2D u_AOTex;
layout(binding = 7) uniform sampler2D u_EmissiveTex;
layout(binding = 9) uniform samplerCube u_IrradianceMap;
layout(binding = 10) uniform samplerCube u_PrefilterMap;
layout(binding = 11) uniform sampler2D u_BRDFLUT;

out vec4 fragColor;

vec3 GetMaterialNormal()
{
    vec3 n = normalize(vWorldNormal);
    vec3 t = normalize(vWorldTangent - n * dot(n, vWorldTangent));
    vec3 b = normalize(cross(n, t));
    mat3 tbn = mat3(t, b, n);
    vec3 tangentNormal = texture(u_NormalTex, vTexCoord).xyz * 2.0 - 1.0;
    return normalize(tbn * tangentNormal);
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

    vec3 numerator = ndf * g * f;
    float denominator = max(4.0 * nDotV * nDotL, 0.0001);
    vec3 specular = numerator / denominator;

    vec3 kS = f;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    return (kD * baseColor / PI + specular) * radiance * nDotL;
}

void main()
{
    vec3 baseColor = texture(u_BaseColorTex, vTexCoord).rgb * u_BaseColorFactor_Metallic.rgb * vColor;
    float metallic = clamp(texture(u_MetallicTex, vTexCoord).r * u_BaseColorFactor_Metallic.w, 0.0, 1.0);
    float roughness = clamp(texture(u_RoughnessTex, vTexCoord).r * u_RoughnessAOEmissiveStrength_Pad.x, 0.04, 1.0);
    float ao = clamp(texture(u_AOTex, vTexCoord).r * u_RoughnessAOEmissiveStrength_Pad.y, 0.0, 1.0);
    vec3 emissive = texture(u_EmissiveTex, vTexCoord).rgb *
                    u_EmissiveFactor_Pad.rgb *
                    u_RoughnessAOEmissiveStrength_Pad.z;

    vec3 n = GetMaterialNormal();
    vec3 v = normalize(u_CameraPosition_Pad.xyz - vWorldPosition);
    vec3 f0 = mix(vec3(0.04), baseColor, metallic);
    float nDotV = max(dot(n, v), 0.0);
    vec3 f = FresnelSchlickRoughness(nDotV, f0, roughness);
    vec3 kS = f;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    vec3 irradiance = texture(u_IrradianceMap, n).rgb;
    vec3 diffuseIBL = irradiance * baseColor;
    vec3 reflection = reflect(-v, n);
    vec3 prefilteredColor = textureLod(u_PrefilterMap, reflection, roughness * 4.0).rgb;
    vec2 brdf = texture(u_BRDFLUT, vec2(nDotV, roughness)).rg;
    vec3 specularIBL = prefilteredColor * (f * brdf.x + brdf.y);
    vec3 color = (kD * diffuseIBL + specularIBL) * ao;

    for (int i = 0; i < u_LightCounts.x; ++i)
    {
        vec3 l = normalize(u_DirectionalLightDirections[i].xyz);
        color += EvaluatePBRLight(n, v, l, u_DirectionalLightColors[i].xyz, baseColor, metallic, roughness);
    }

    for (int i = 0; i < u_LightCounts.y; ++i)
    {
        vec3 lightVector = u_PointLightPositions[i].xyz - vWorldPosition;
        float distanceToLight = length(lightVector);
        float radius = max(u_PointLightColorsAndRadii[i].w, 0.001);
        vec3 l = lightVector / max(distanceToLight, 0.001);
        float attenuation = clamp(1.0 - distanceToLight / radius, 0.0, 1.0);
        attenuation *= attenuation;
        vec3 radiance = u_PointLightColorsAndRadii[i].xyz * attenuation;
        color += EvaluatePBRLight(n, v, l, radiance, baseColor, metallic, roughness);
    }

    fragColor = vec4(color + emissive, 1.0);
}
