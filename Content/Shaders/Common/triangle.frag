#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 0,binding = 1) uniform TestUBO {
    vec4 a;
    float b;
    vec2 c;
    float _padding;
} test;
layout(set = 0, binding = 2) uniform dynamicUBO{
    vec4 x;
} dynamicUbo;
// 推送常量：控制是否使用纹理
layout(push_constant) uniform PushConstants {
    uint useTexture;  // 0 = 不使用纹理, 1 = 使用纹理
} pushConstants;

layout(location = 0) out vec4 outColor;

void main() {
    if (pushConstants.useTexture == 1) {
        // 使用纹理
        outColor = texture(texSampler, fragTexCoord);
    } else {
        // 不使用纹理，使用顶点颜色或固定颜色
        //outColor = vec4(fragNormal * 0.5 + 0.5, 1.0);  // 使用法线作为颜色
        outColor = dynamicUbo.x;
    }
}
