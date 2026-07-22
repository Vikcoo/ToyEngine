// ToyEngine RHIVulkan Module
// RHI 与 Vulkan 基础枚举转换

#pragma once

#include "RHITypes.h"

#include <vulkan/vulkan.h>

namespace TE {

[[nodiscard]] inline VkFormat ToVulkanFormat(const RHIFormat format, const bool srgb = false)
{
    switch (format)
    {
    case RHIFormat::Float: return VK_FORMAT_R32_SFLOAT;
    case RHIFormat::Float2: return VK_FORMAT_R32G32_SFLOAT;
    case RHIFormat::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
    case RHIFormat::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case RHIFormat::Int: return VK_FORMAT_R32_SINT;
    case RHIFormat::Int2: return VK_FORMAT_R32G32_SINT;
    case RHIFormat::Int3: return VK_FORMAT_R32G32B32_SINT;
    case RHIFormat::Int4: return VK_FORMAT_R32G32B32A32_SINT;
    case RHIFormat::UInt: return VK_FORMAT_R32_UINT;
    case RHIFormat::UInt2: return VK_FORMAT_R32G32_UINT;
    case RHIFormat::UInt3: return VK_FORMAT_R32G32B32_UINT;
    case RHIFormat::UInt4: return VK_FORMAT_R32G32B32A32_UINT;
    case RHIFormat::R8_UNorm: return VK_FORMAT_R8_UNORM;
    case RHIFormat::RG8_UNorm: return VK_FORMAT_R8G8_UNORM;
    case RHIFormat::RGBA8_UNorm: return srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    case RHIFormat::BGRA8_UNorm: return srgb ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM;
    case RHIFormat::RGBA8_sRGB: return VK_FORMAT_R8G8B8A8_SRGB;
    case RHIFormat::BGRA8_sRGB: return VK_FORMAT_B8G8R8A8_SRGB;
    case RHIFormat::RGBA32_Float: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case RHIFormat::D16_UNorm: return VK_FORMAT_D16_UNORM;
    case RHIFormat::D24_UNorm_S8_UInt: return VK_FORMAT_D24_UNORM_S8_UINT;
    case RHIFormat::D32_Float: return VK_FORMAT_D32_SFLOAT;
    default: return VK_FORMAT_UNDEFINED;
    }
}

[[nodiscard]] inline VkShaderStageFlags ToVulkanShaderStages(const RHIShaderStage stage)
{
    switch (stage)
    {
    case RHIShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
    case RHIShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
    case RHIShaderStage::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
    case RHIShaderStage::TessControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case RHIShaderStage::TessEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case RHIShaderStage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
    }
    return 0;
}

[[nodiscard]] inline VkDescriptorType ToVulkanDescriptorType(const RHIBindingType type)
{
    switch (type)
    {
    case RHIBindingType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case RHIBindingType::DynamicUniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    case RHIBindingType::Texture2D:
    case RHIBindingType::TextureCube: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case RHIBindingType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
    }
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

[[nodiscard]] inline VkSampleCountFlagBits ToVulkanSampleCount(const RHISampleCount count)
{
    return static_cast<VkSampleCountFlagBits>(static_cast<uint8_t>(count));
}

} // namespace TE
