#version 450 core

in vec3 vWorldNormal;
in vec3 vWorldTangent;
in vec3 vWorldPosition;
in vec2 vTexCoord;
in vec3 vColor;

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

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outWorldPosition;
layout(location = 3) out vec4 outMaterial;

vec3 GetMaterialNormal()
{
    vec3 n = normalize(vWorldNormal);
    vec3 t = normalize(vWorldTangent - n * dot(n, vWorldTangent));
    vec3 b = normalize(cross(n, t));
    mat3 tbn = mat3(t, b, n);
    vec3 tangentNormal = texture(u_NormalTex, vTexCoord).xyz * 2.0 - 1.0;
    return normalize(tbn * tangentNormal);
}

void main()
{
    vec3 baseColor = texture(u_BaseColorTex, vTexCoord).rgb * u_BaseColorFactor_Metallic.rgb * vColor;
    vec3 encodedNormal = GetMaterialNormal() * 0.5 + 0.5;
    float metallic = clamp(texture(u_MetallicTex, vTexCoord).r * u_BaseColorFactor_Metallic.w, 0.0, 1.0);
    float roughness = clamp(texture(u_RoughnessTex, vTexCoord).r * u_RoughnessAOEmissiveStrength_Pad.x, 0.04, 1.0);
    float ao = clamp(texture(u_AOTex, vTexCoord).r * u_RoughnessAOEmissiveStrength_Pad.y, 0.0, 1.0);
    vec3 emissive = texture(u_EmissiveTex, vTexCoord).rgb *
                    u_EmissiveFactor_Pad.rgb *
                    u_RoughnessAOEmissiveStrength_Pad.z;
    float emissiveLuma = max(max(emissive.r, emissive.g), emissive.b);

    outAlbedo = vec4(baseColor, 1.0);
    outNormal = vec4(encodedNormal, 1.0);
    outWorldPosition = vec4(vWorldPosition, 1.0);
    outMaterial = vec4(metallic, roughness, ao, emissiveLuma);
}
