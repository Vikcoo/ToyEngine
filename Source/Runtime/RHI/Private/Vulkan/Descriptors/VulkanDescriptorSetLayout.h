// Vulkan Descriptor Set Layout - 描述符集布局
#pragma once

#include "../Core/VulkanDevice.h"
#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include <memory>

namespace TE {

class VulkanDevice;

/// 描述符集布局绑定配置
struct DescriptorSetLayoutBinding {
    uint32_t binding;                    // 绑定索引（对应 shader 中的 binding = X）
    vk::DescriptorType descriptorType;   // 描述符类型（如 eUniformBuffer）
    uint32_t descriptorCount = 1;        // 描述符数量
    vk::ShaderStageFlags stageFlags;     // 使用的着色器阶段
};

/// Vulkan 描述符集布局
class VulkanDescriptorSetLayout {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanDevice;
    };

    explicit VulkanDescriptorSetLayout(
        PrivateTag,
        std::shared_ptr<VulkanDevice> device,
        const std::vector<DescriptorSetLayoutBinding>& bindings
    );

    ~VulkanDescriptorSetLayout();

    // 禁用拷贝，允许移动
    VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
    VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;
    VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&&) noexcept = default;
    VulkanDescriptorSetLayout& operator=(VulkanDescriptorSetLayout&&) noexcept = default;

    // 获取句柄
    [[nodiscard]] const vk::raii::DescriptorSetLayout& GetHandle() const { return m_layout; }

private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::DescriptorSetLayout m_layout{nullptr};
};

} // namespace TE

