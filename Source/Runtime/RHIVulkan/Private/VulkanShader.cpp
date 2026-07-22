// ToyEngine RHIVulkan Module
// Vulkan ShaderModule 实现

#include "VulkanShader.h"

#include "VulkanDevice.h"
#include "Log/Log.h"

#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace TE {

namespace {

[[nodiscard]] std::filesystem::path ResolveShaderPath(const std::string& logicalName)
{
    static const std::unordered_map<std::string, std::string> mappings = {
        {"StaticMesh/StageBValidationVS", "stage_b_validation.vert.spv"},
        {"StaticMesh/StageBValidationPS", "stage_b_validation.frag.spv"},
        {"StaticMesh/BasePassVS", "model.vert.spv"},
        {"StaticMesh/BasePassPS", "model.frag.spv"},
        {"StaticMesh/GBufferVS", "gbuffer.vert.spv"},
        {"StaticMesh/GBufferPS", "gbuffer.frag.spv"},
        {"Deferred/LightingVS", "deferred_lighting.vert.spv"},
        {"Deferred/LightingPS", "deferred_lighting.frag.spv"},
        {"Sky/FullscreenVS", "deferred_lighting.vert.spv"},
        {"Sky/SkyPS", "sky.frag.spv"},
    };
    const auto it = mappings.find(logicalName);
    if (it == mappings.end())
    {
        return {};
    }
    return std::filesystem::path(TE_VULKAN_SHADER_ROOT_DIR) / it->second;
}

} // namespace

VulkanShader::VulkanShader(VulkanDevice& device, const RHIShaderDesc& desc)
    : m_Device(&device)
    , m_Stage(desc.stage)
    , m_EntryPoint(desc.entryPoint)
{
    const std::filesystem::path path = ResolveShaderPath(desc.logicalName);
    if (path.empty())
    {
        TE_LOG_ERROR("[RHIVulkan] Unknown logical shader: {}", desc.logicalName);
        return;
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
    {
        TE_LOG_ERROR("[RHIVulkan] Cannot open SPIR-V: {}", path.string());
        return;
    }
    const std::streamsize byteCount = file.tellg();
    if (byteCount <= 0 || (byteCount % 4) != 0)
    {
        TE_LOG_ERROR("[RHIVulkan] Invalid SPIR-V byte size: {}", path.string());
        return;
    }
    std::vector<uint32_t> code(static_cast<size_t>(byteCount) / sizeof(uint32_t));
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(code.data()), byteCount))
    {
        TE_LOG_ERROR("[RHIVulkan] Failed to read SPIR-V: {}", path.string());
        return;
    }

    const VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = static_cast<size_t>(byteCount),
        .pCode = code.data(),
    };
    if (vkCreateShaderModule(device.GetNativeDevice(), &createInfo, nullptr, &m_Module) != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] ShaderModule creation failed: {}", desc.debugName);
    }
}

VulkanShader::~VulkanShader()
{
    if (m_Device && m_Module != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_Device->GetNativeDevice(), m_Module, nullptr);
    }
}

} // namespace TE
