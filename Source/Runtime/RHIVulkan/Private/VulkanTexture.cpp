// ToyEngine RHIVulkan Module
// Vulkan Texture 实现

#include "VulkanTexture.h"

#include "VulkanConversions.h"
#include "VulkanDevice.h"
#include "Log/Log.h"

#include <vector>

namespace TE {

namespace {

struct FNativeImageState
{
    VkImageLayout Layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkPipelineStageFlags Stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags Access = 0;
};

[[nodiscard]] FNativeImageState ToNativeState(const RHIResourceState state)
{
    switch (state)
    {
    case RHIResourceState::CopySource:
        return {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT};
    case RHIResourceState::CopyDestination:
        return {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT};
    case RHIResourceState::RenderTarget:
        return {VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
    case RHIResourceState::DepthWrite:
        return {VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
    case RHIResourceState::DepthRead:
        return {VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT};
    case RHIResourceState::ShaderResource:
        return {VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT};
    case RHIResourceState::Present:
        return {VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0};
    case RHIResourceState::Undefined:
    default:
        return {};
    }
}

[[nodiscard]] VkImageUsageFlags ToVulkanImageUsage(const RHITextureUsage usage)
{
    VkImageUsageFlags result = 0;
    if (HasAnyFlags(usage, RHITextureUsage::ShaderResource)) result |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (HasAnyFlags(usage, RHITextureUsage::ColorAttachment)) result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (HasAnyFlags(usage, RHITextureUsage::DepthStencilAttachment)) result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (HasAnyFlags(usage, RHITextureUsage::CopySource)) result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (HasAnyFlags(usage, RHITextureUsage::CopyDestination)) result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (HasAnyFlags(usage, RHITextureUsage::Storage)) result |= VK_IMAGE_USAGE_STORAGE_BIT;
    return result;
}

} // namespace

VulkanTexture::VulkanTexture(VulkanDevice& device, const RHITextureDesc& desc)
    : m_Device(&device)
    , m_Width(desc.width)
    , m_Height(desc.height)
    , m_Format(desc.format)
    , m_Usage(desc.usage)
    , m_SampleCount(desc.sampleCount)
{
    if (desc.dimension != RHITextureDimension::Texture2D || desc.width == 0 || desc.height == 0)
    {
        TE_LOG_ERROR("[RHIVulkan] Stage B only supports non-empty Texture2D: {}", desc.debugName);
        return;
    }
    const uint32_t mipLevels = desc.mipLevels == 0 ? (desc.generateMips ? 0u : 1u) : desc.mipLevels;
    if (mipLevels != 1)
    {
        TE_LOG_ERROR("[RHIVulkan] Stage B only supports one texture mip: {}", desc.debugName);
        return;
    }

    m_NativeFormat = ToVulkanFormat(desc.format, desc.srgb);
    if (m_NativeFormat == VK_FORMAT_UNDEFINED)
    {
        TE_LOG_ERROR("[RHIVulkan] Unsupported texture format: {}", desc.debugName);
        return;
    }
    const bool isDepth = HasAnyFlags(desc.usage, RHITextureUsage::DepthStencilAttachment);
    m_AspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageUsageFlags usage = ToVulkanImageUsage(desc.usage);
    if (desc.initialData) usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (!device.CreateImageAllocation(desc.width, desc.height, m_NativeFormat, usage,
                                      ToVulkanSampleCount(desc.sampleCount), m_Image, m_Memory))
    {
        TE_LOG_ERROR("[RHIVulkan] Image allocation failed: {}", desc.debugName);
        return;
    }

    const VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = m_Image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = m_NativeFormat,
        .subresourceRange = {m_AspectMask, 0, 1, 0, 1},
    };
    if (vkCreateImageView(device.GetNativeDevice(), &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] ImageView creation failed: {}", desc.debugName);
        return;
    }

    if (desc.initialData)
    {
        const uint64_t tightRowPitch = static_cast<uint64_t>(desc.width) * GetFormatSize(desc.format);
        const uint64_t rowPitch = desc.initialDataRowPitch == 0 ? tightRowPitch : desc.initialDataRowPitch;
        const uint64_t dataSize = desc.initialDataSize == 0 ? rowPitch * desc.height : desc.initialDataSize;
        if (!device.UploadTexture2D(m_Image, desc.width, desc.height, m_AspectMask,
                                    desc.initialData, dataSize, rowPitch, tightRowPitch))
        {
            TE_LOG_ERROR("[RHIVulkan] Texture upload failed: {}", desc.debugName);
            return;
        }
        m_Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    else if (desc.initialState != RHIResourceState::Undefined)
    {
        const FNativeImageState target = ToNativeState(desc.initialState);
        const bool transitioned = device.ExecuteImmediate([&](const VkCommandBuffer commandBuffer)
        {
            RecordTransition(commandBuffer, RHIResourceState::Undefined, desc.initialState);
        });
        if (!transitioned)
        {
            TE_LOG_ERROR("[RHIVulkan] Initial texture transition failed: {}", desc.debugName);
            return;
        }
        m_Layout = target.Layout;
    }
    m_Valid = true;
}

VulkanTexture::~VulkanTexture()
{
    if (!m_Device) return;
    const VkDevice device = m_Device->GetNativeDevice();
    if (m_ImageView != VK_NULL_HANDLE) vkDestroyImageView(device, m_ImageView, nullptr);
    if (m_Image != VK_NULL_HANDLE) vkDestroyImage(device, m_Image, nullptr);
    if (m_Memory != VK_NULL_HANDLE) vkFreeMemory(device, m_Memory, nullptr);
}

void VulkanTexture::RecordTransition(const VkCommandBuffer commandBuffer,
                                     const RHIResourceState before,
                                     const RHIResourceState after)
{
    const FNativeImageState source = ToNativeState(before);
    const FNativeImageState destination = ToNativeState(after);
    const VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = source.Access,
        .dstAccessMask = destination.Access,
        .oldLayout = source.Layout,
        .newLayout = destination.Layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_Image,
        .subresourceRange = {m_AspectMask, 0, 1, 0, 1},
    };
    vkCmdPipelineBarrier(commandBuffer, source.Stage, destination.Stage, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);
    m_Layout = destination.Layout;
}

} // namespace TE
