// Vulkan Shader 实现
#include "VulkanShader.h"
#include "VulkanDevice.h"
#include "Log/Log.h"
#include <fstream>
#include <algorithm>

namespace TE {

VulkanShader::VulkanShader(PrivateTag,
                          std::shared_ptr<VulkanDevice> device,
                          const std::string& shaderFilePath)
    : m_device(std::move(device))
    , m_stage(DetermineShaderStage(shaderFilePath))
{
    const auto shaderCode = ReadShaderFile(shaderFilePath);
    
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.setCode(shaderCode);

    m_shaderModule = m_device->GetHandle().createShaderModule(createInfo);
    if (m_shaderModule == nullptr){
        TE_LOG_ERROR("Failed to create shader module");
    }
    TE_LOG_DEBUG("Shader module created: {} ({})", shaderFilePath, vk::to_string(m_stage));
}

VulkanShader::~VulkanShader() {
    TE_LOG_DEBUG("Shader module destroyed");
}

vk::ShaderStageFlagBits VulkanShader::DetermineShaderStage(const std::string& filePath) {
    // 查找最后一个点号的位置
    const size_t lastDot = filePath.find_last_of('.');
    if (lastDot == std::string::npos) {
        TE_LOG_WARN("No extension found in shader file: {}, defaulting to vertex", filePath);
        return vk::ShaderStageFlagBits::eVertex;
    }
    
    // 获取扩展名（包括点号）
    std::string ext = filePath.substr(lastDot);
    
    // 对于 .spv 文件，需要检查文件名来确定类型
    if (ext == ".spv") {
        // 检查文件名中包含的类型标识
        const std::string filename = filePath.substr(filePath.find_last_of("/\\") + 1);
        if (filename.find("vert") != std::string::npos) {
            return vk::ShaderStageFlagBits::eVertex;
        } else if (filename.find("frag") != std::string::npos) {
            return vk::ShaderStageFlagBits::eFragment;
        } else if (filename.find("geom") != std::string::npos) {
            return vk::ShaderStageFlagBits::eGeometry;
        } else if (filename.find("tesc") != std::string::npos) {
            return vk::ShaderStageFlagBits::eTessellationControl;
        } else if (filename.find("tese") != std::string::npos) {
            return vk::ShaderStageFlagBits::eTessellationEvaluation;
        } else if (filename.find("comp") != std::string::npos) {
            return vk::ShaderStageFlagBits::eCompute;
        }
        // 如果无法从文件名判断，默认使用顶点着色器
        TE_LOG_WARN("Cannot determine shader stage from .spv filename: {}, defaulting to vertex", filename);
        return vk::ShaderStageFlagBits::eVertex;
    }
    
    // 处理其他扩展名
    if (ext == ".vert" || ext == ".vertex") {
        return vk::ShaderStageFlagBits::eVertex;
    } else if (ext == ".frag" || ext == ".fragment") {
        return vk::ShaderStageFlagBits::eFragment;
    } else if (ext == ".geom" || ext == ".geometry") {
        return vk::ShaderStageFlagBits::eGeometry;
    } else if (ext == ".tesc" || ext == ".tessellation_control") {
        return vk::ShaderStageFlagBits::eTessellationControl;
    } else if (ext == ".tese" || ext == ".tessellation_evaluation") {
        return vk::ShaderStageFlagBits::eTessellationEvaluation;
    } else if (ext == ".comp" || ext == ".compute") {
        return vk::ShaderStageFlagBits::eCompute;
    }
    
    TE_LOG_WARN("Unknown shader extension: {}, defaulting to vertex", ext);
    return vk::ShaderStageFlagBits::eVertex;
}

std::vector<uint32_t> VulkanShader::ReadShaderFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        TE_LOG_ERROR("Failed to open shader file: {}", filePath);
        throw std::runtime_error("Failed to open shader file");
    }
    
    const size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();
    
    return buffer;
}

} // namespace TE
