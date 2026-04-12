#version 410 core

// 从顶点着色器传入的 varying
in vec3 vWorldNormal;   // 世界空间法线
in vec2 vTexCoord;      // 纹理坐标（预留，暂未使用纹理）
in vec3 vColor;         // 顶点色

// 光照参数 Uniform
uniform vec3 u_LightDir;    // 方向光的方向（指向光源，已归一化）
uniform vec3 u_LightColor;  // 光源颜色
uniform sampler2D u_BaseColorTex; // BaseColor 贴图（无贴图时绑定白纹理）

// 输出
out vec4 fragColor;

void main() {
    // 归一化法线（插值后可能不再是单位向量）
    vec3 normal = normalize(vWorldNormal);

    // === 环境光 ===
    // 环境光 = 环境光系数 × 光源颜色
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * u_LightColor;

    // === 漫反射（Lambertian） ===
    // diffuse = max(dot(N, L), 0) × 光源颜色
    float NdotL = max(dot(normal, u_LightDir), 0.0);
    vec3 diffuse = NdotL * u_LightColor;

    // 纹理颜色与顶点色相乘作为基础颜色
    vec3 baseColor = texture(u_BaseColorTex, vTexCoord).rgb * vColor;

    // 最终颜色 = (环境光 + 漫反射) × 基础颜色
    vec3 result = (ambient + diffuse) * baseColor;

    fragColor = vec4(result, 1.0);
}
