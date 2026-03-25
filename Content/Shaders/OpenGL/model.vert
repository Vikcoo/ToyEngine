#version 410 core

// 顶点属性输入（与 FStaticMeshVertex 布局对应）
layout(location = 0) in vec3 aPosition;     // 模型空间位置
layout(location = 1) in vec3 aNormal;       // 模型空间法线
layout(location = 2) in vec2 aTexCoord;     // 纹理坐标
layout(location = 3) in vec3 aColor;        // 顶点色

// 变换矩阵 Uniform
uniform mat4 u_MVP;     // Model-View-Projection 矩阵（投影用）
uniform mat4 u_Model;   // Model 矩阵（世界变换，用于法线变换到世界空间）

// 传递给片段着色器的 varying
out vec3 vWorldNormal;  // 世界空间法线
out vec2 vTexCoord;     // 纹理坐标
out vec3 vColor;        // 顶点色

void main() {
    gl_Position = u_MVP * vec4(aPosition, 1.0);

    // 将法线从模型空间变换到世界空间
    // 使用 mat3(u_Model) 近似（忽略非均匀缩放的情况，简化版）
    // 严格做法应使用逆转置矩阵，但对均匀缩放足够
    vWorldNormal = normalize(mat3(u_Model) * aNormal);

    vTexCoord = aTexCoord;
    vColor = aColor;
}
