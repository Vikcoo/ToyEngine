// Vulkan 顶点输入辅助函数
#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include <cstddef>

namespace TE {

/// 顶点输入辅助工具
/// 用于从顶点结构体自动生成顶点输入绑定和属性描述
template<typename VertexType>
struct VertexInputHelper {
    // 获取顶点输入绑定描述
    // binding: 绑定索引（默认为0）
    static vk::VertexInputBindingDescription GetBindingDescription(uint32_t binding = 0) {
        vk::VertexInputBindingDescription bindingDescription;
        bindingDescription.setBinding(binding)
                          .setStride(sizeof(VertexType))
                          .setInputRate(vk::VertexInputRate::eVertex);
        return bindingDescription;
    }
    
    // 获取顶点输入属性描述
    // 注意：需要为每个具体的顶点类型特化此函数
    // binding: 绑定索引（默认为0）
    static std::vector<vk::VertexInputAttributeDescription> GetAttributeDescriptions(uint32_t binding = 0);
};

} // namespace TE

