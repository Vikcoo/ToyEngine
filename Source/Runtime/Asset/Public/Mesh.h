//
// Created by yukai on 2026/1/3.
//

#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "vulkan/vulkan.hpp"

namespace TE {

// 前向声明
class VulkanDevice;
class VulkanBuffer;

/// 顶点结构（与 Sandbox 中的定义一致）
// 定义顶点结构体
struct Vertex {
    glm::vec3 pos;  // 位置 (location = 0)
    glm::vec3 nor;     // 颜色 (location = 1)
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription;
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;

        return bindingDescription;
    }

    static std::vector<vk::VertexInputAttributeDescription>  getAttributeDescriptions() {
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
        attributeDescriptions.emplace_back(0 , 0 , vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, pos)));
        attributeDescriptions.emplace_back(1 , 0 , vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, nor)));
        attributeDescriptions.emplace_back(2 , 0 , vk::Format::eR32G32Sfloat, static_cast<uint32_t>(offsetof(Vertex, texCoord)));
        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return (this->pos == other.pos) && (this->texCoord == other.texCoord) && (this->nor == other.nor);
    }
};

/// Mesh 资源类 - 包含顶点和索引数据
class Mesh {
public:
    Mesh() = default;
    ~Mesh() = default;

    // 禁用拷贝，允许移动
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    bool isValid() const { return m_isValid; }


    std::string name;
    bool m_isValid = false;
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
};

} // namespace TE