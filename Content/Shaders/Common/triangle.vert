#version 450



layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inInstanceMat0;  // mat4 第 0 列
layout(location = 4) in vec4 inInstanceMat1;  // mat4 第 1 列
layout(location = 5) in vec4 inInstanceMat2;  // mat4 第 2 列
layout(location = 6) in vec4 inInstanceMat3;  // mat4 第 3 列
layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;
layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;



void main() {
    // 重建实例模型矩阵
    mat4 instanceModel = mat4(
        inInstanceMat0,
        inInstanceMat1,
        inInstanceMat2,
        inInstanceMat3
    );
    gl_Position = ubo.proj * ubo.view * instanceModel * vec4(inPosition, 1.0);
    fragNormal = mat3(transpose(inverse(instanceModel))) * inNormal;
    fragTexCoord = inTexCoord;
}