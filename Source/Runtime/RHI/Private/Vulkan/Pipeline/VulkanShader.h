// Vulkan Shader - 着色器模块
#pragma once

#include "../Core/VulkanDevice.h"
#include <vulkan/vulkan_raii.hpp>
#include <string>
#include <memory>

namespace TE {

class VulkanDevice;

/// Vulkan 着色器模块 - 管理着色器代码
class VulkanShader {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanDevice;
        friend class VulkanPipeline;
    };

    VulkanShader(PrivateTag,
                 std::shared_ptr<VulkanDevice> device,
                 const std::string& shaderFilePath);

    ~VulkanShader();

    // 禁用拷贝，允许移动
    VulkanShader(const VulkanShader&) = delete;
    VulkanShader& operator=(const VulkanShader&) = delete;
    VulkanShader(VulkanShader&&) noexcept = default;
    VulkanShader& operator=(VulkanShader&&) noexcept = default;

    // 获取句柄
    [[nodiscard]] const vk::raii::ShaderModule& GetHandle() const { return m_shaderModule; }
    [[nodiscard]] vk::ShaderStageFlagBits GetStage() const { return m_stage; }

private:
    static vk::ShaderStageFlagBits DetermineShaderStage(const std::string& filePath);
    std::vector<uint32_t> ReadShaderFile(const std::string& filePath);

private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::ShaderModule m_shaderModule{nullptr};
    vk::ShaderStageFlagBits m_stage;
};

} // namespace TE
