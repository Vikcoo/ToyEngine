// ToyEngine RHIVulkan Module
// Vulkan Graphics Pipeline 实现

#include "VulkanPipeline.h"

#include "VulkanConversions.h"
#include "VulkanDescriptors.h"
#include "VulkanDevice.h"
#include "VulkanShader.h"
#include "Log/Log.h"

#include <array>
#include <vector>

namespace TE {

namespace {

[[nodiscard]] VkPrimitiveTopology ToTopology(const RHIPrimitiveTopology value)
{
    switch (value)
    {
    case RHIPrimitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case RHIPrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case RHIPrimitiveTopology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case RHIPrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case RHIPrimitiveTopology::TriangleList:
    default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

[[nodiscard]] VkPolygonMode ToPolygonMode(const RHIPolygonMode value)
{
    switch (value)
    {
    case RHIPolygonMode::Line: return VK_POLYGON_MODE_LINE;
    case RHIPolygonMode::Point: return VK_POLYGON_MODE_POINT;
    case RHIPolygonMode::Fill:
    default: return VK_POLYGON_MODE_FILL;
    }
}

[[nodiscard]] VkCullModeFlags ToCullMode(const RHICullMode value)
{
    switch (value)
    {
    case RHICullMode::None: return VK_CULL_MODE_NONE;
    case RHICullMode::Front: return VK_CULL_MODE_FRONT_BIT;
    case RHICullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
    case RHICullMode::Back:
    default: return VK_CULL_MODE_BACK_BIT;
    }
}

[[nodiscard]] VkCompareOp ToCompareOp(const RHICompareOp value)
{
    return static_cast<VkCompareOp>(static_cast<uint8_t>(value));
}

[[nodiscard]] VkBlendFactor ToBlendFactor(const RHIBlendFactor value)
{
    switch (value)
    {
    case RHIBlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
    case RHIBlendFactor::One: return VK_BLEND_FACTOR_ONE;
    case RHIBlendFactor::SourceColor: return VK_BLEND_FACTOR_SRC_COLOR;
    case RHIBlendFactor::OneMinusSourceColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case RHIBlendFactor::SourceAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
    case RHIBlendFactor::OneMinusSourceAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case RHIBlendFactor::DestinationColor: return VK_BLEND_FACTOR_DST_COLOR;
    case RHIBlendFactor::OneMinusDestinationColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case RHIBlendFactor::DestinationAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
    case RHIBlendFactor::OneMinusDestinationAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    }
    return VK_BLEND_FACTOR_ONE;
}

[[nodiscard]] VkBlendOp ToBlendOp(const RHIBlendOp value)
{
    switch (value)
    {
    case RHIBlendOp::Subtract: return VK_BLEND_OP_SUBTRACT;
    case RHIBlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
    case RHIBlendOp::Min: return VK_BLEND_OP_MIN;
    case RHIBlendOp::Max: return VK_BLEND_OP_MAX;
    case RHIBlendOp::Add:
    default: return VK_BLEND_OP_ADD;
    }
}

[[nodiscard]] VkColorComponentFlags ToColorWriteMask(const RHIColorWriteMask value)
{
    VkColorComponentFlags result = 0;
    if (HasAnyFlags(value, RHIColorWriteMask::Red)) result |= VK_COLOR_COMPONENT_R_BIT;
    if (HasAnyFlags(value, RHIColorWriteMask::Green)) result |= VK_COLOR_COMPONENT_G_BIT;
    if (HasAnyFlags(value, RHIColorWriteMask::Blue)) result |= VK_COLOR_COMPONENT_B_BIT;
    if (HasAnyFlags(value, RHIColorWriteMask::Alpha)) result |= VK_COLOR_COMPONENT_A_BIT;
    return result;
}

} // namespace

VulkanPipeline::VulkanPipeline(VulkanDevice& device, const RHIPipelineDesc& desc)
    : m_Device(&device)
{
    auto* vertexShader = static_cast<VulkanShader*>(desc.vertexShader);
    auto* fragmentShader = static_cast<VulkanShader*>(desc.fragmentShader);
    m_Layout = static_cast<VulkanPipelineLayout*>(desc.layout);
    if (!vertexShader || !vertexShader->IsValid() || !fragmentShader || !fragmentShader->IsValid() ||
        !m_Layout || !m_Layout->IsValid() || desc.rendering.colorAttachmentFormats.empty())
    {
        TE_LOG_ERROR("[RHIVulkan] Invalid Graphics Pipeline description: {}", desc.debugName);
        return;
    }

    const std::array shaderStages = {
        VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                                        VK_SHADER_STAGE_VERTEX_BIT, vertexShader->GetHandle(),
                                        vertexShader->GetEntryPoint().c_str(), nullptr},
        VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                                        VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader->GetHandle(),
                                        fragmentShader->GetEntryPoint().c_str(), nullptr},
    };

    std::vector<VkVertexInputBindingDescription> bindings;
    for (const auto& binding : desc.vertexInput.bindings)
    {
        bindings.push_back({binding.binding, binding.stride, VK_VERTEX_INPUT_RATE_VERTEX});
    }
    std::vector<VkVertexInputAttributeDescription> attributes;
    for (const auto& attribute : desc.vertexInput.attributes)
    {
        const VkFormat format = ToVulkanFormat(attribute.format);
        if (format == VK_FORMAT_UNDEFINED)
        {
            TE_LOG_ERROR("[RHIVulkan] Invalid vertex format in {}", desc.debugName);
            return;
        }
        attributes.push_back({attribute.location, 0, format, attribute.offset});
    }
    const VkPipelineVertexInputStateCreateInfo vertexInput{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size()),
        .pVertexBindingDescriptions = bindings.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size()),
        .pVertexAttributeDescriptions = attributes.data(),
    };
    const VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = ToTopology(desc.topology),
    };
    const VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };
    const VkPipelineRasterizationStateCreateInfo rasterization{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = desc.rasterization.depthClampEnable,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = ToPolygonMode(desc.rasterization.polygonMode),
        .cullMode = ToCullMode(desc.rasterization.cullMode),
        // 投影矩阵已翻转 Y，必须同步翻转 winding 才能保持 RHI 的 CCW 语义。
        .frontFace = desc.rasterization.frontFace == RHIFrontFace::CounterClockwise
            ? VK_FRONT_FACE_CLOCKWISE
            : VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = desc.rasterization.lineWidth,
    };
    const VkPipelineMultisampleStateCreateInfo multisample{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = ToVulkanSampleCount(desc.rendering.sampleCount),
    };
    const VkPipelineDepthStencilStateCreateInfo depthStencil{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = desc.depthStencil.depthTestEnable,
        .depthWriteEnable = desc.depthStencil.depthWriteEnable,
        .depthCompareOp = ToCompareOp(desc.depthStencil.depthCompareOp),
    };

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
    blendAttachments.reserve(desc.rendering.colorAttachmentFormats.size());
    for (size_t index = 0; index < desc.rendering.colorAttachmentFormats.size(); ++index)
    {
        const RHIColorBlendAttachmentDesc blend = index < desc.rendering.colorBlendAttachments.size()
            ? desc.rendering.colorBlendAttachments[index]
            : RHIColorBlendAttachmentDesc{};
        blendAttachments.push_back({
            .blendEnable = blend.blendEnable,
            .srcColorBlendFactor = ToBlendFactor(blend.sourceColorFactor),
            .dstColorBlendFactor = ToBlendFactor(blend.destinationColorFactor),
            .colorBlendOp = ToBlendOp(blend.colorBlendOp),
            .srcAlphaBlendFactor = ToBlendFactor(blend.sourceAlphaFactor),
            .dstAlphaBlendFactor = ToBlendFactor(blend.destinationAlphaFactor),
            .alphaBlendOp = ToBlendOp(blend.alphaBlendOp),
            .colorWriteMask = ToColorWriteMask(blend.writeMask),
        });
    }
    const VkPipelineColorBlendStateCreateInfo colorBlend{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(blendAttachments.size()),
        .pAttachments = blendAttachments.data(),
    };

    const std::array dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    const VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };
    std::vector<VkFormat> colorFormats;
    for (const RHIFormat format : desc.rendering.colorAttachmentFormats)
    {
        colorFormats.push_back(ToVulkanFormat(format));
    }
    const VkPipelineRenderingCreateInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = static_cast<uint32_t>(colorFormats.size()),
        .pColorAttachmentFormats = colorFormats.data(),
        .depthAttachmentFormat = ToVulkanFormat(desc.rendering.depthStencilFormat),
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };
    const VkGraphicsPipelineCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingInfo,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisample,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlend,
        .pDynamicState = &dynamicState,
        .layout = m_Layout->GetHandle(),
    };
    if (vkCreateGraphicsPipelines(device.GetNativeDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] Graphics Pipeline creation failed: {}", desc.debugName);
    }
}

VulkanPipeline::~VulkanPipeline()
{
    if (m_Device && m_Pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_Device->GetNativeDevice(), m_Pipeline, nullptr);
    }
}

} // namespace TE
