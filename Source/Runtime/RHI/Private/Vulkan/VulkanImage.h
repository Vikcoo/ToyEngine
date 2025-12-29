//
// Created by yukai on 2025/12/30.
//

#pragma once
#include <memory>
#include <vulkan/vulkan.hpp>

#include "VulkanImageView.h"


namespace TE
{
    struct VulkanImageConfig{
        uint32_t width;
        uint32_t height;
        vk::Format format;
        vk::ImageTiling tiling;
        vk::ImageUsageFlags usage;
        vk::MemoryPropertyFlags properties;
    };

class VulkanImage
{
public:
    struct PrivateTag{
    private:
        PrivateTag() = default;
        friend class VulkanImage;
    };

    VulkanImage(const VulkanImage&) = delete;
    VulkanImage& operator=(const VulkanImage&) = delete;
    VulkanImage(VulkanImage&&) = delete;
    VulkanImage& operator=(VulkanImage&&) = delete;

    VulkanImage(PrivateTag , std::shared_ptr<VulkanDevice> device, const VulkanImageConfig& config);

    static std::unique_ptr<VulkanImage>Create(std::shared_ptr<VulkanDevice> device, const VulkanImageConfig& config);

    const vk::raii::Image& GetHandle(){ return m_image;}
private:
    vk::raii::Image m_image{nullptr};
    vk::raii::DeviceMemory m_memory{nullptr};
    std::shared_ptr<VulkanDevice> m_device;
};

} // TE

