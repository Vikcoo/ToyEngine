//
// Created by yukai on 2025/12/30.
//

#pragma once
#include <cstdint>
#include <memory>
#include <vector>


namespace TE
{
    class VulkanTexture2D;

    // 通用纹理数据结构体（与API无关，仅存储CPU端原始数据）
    struct RawTextureData {
        uint32_t m_width = 0;          // 纹理宽度
        uint32_t m_height = 0;         // 纹理高度
        uint32_t m_mipLevelCount = 1;  // mipmap层级数
        uint32_t m_channelCount = 4;   // 通道数（3=RGB，4=RGBA，1=灰度等）
        size_t m_dataSize = 0;         // 总数据大小（字节）

        std::vector<uint8_t> m_pixelData;  // 原始数据（CPU可访问，存储格式为：RGBA8888 或 RGB888 等通用格式）

        enum class TextureType {
            Texture2D,    // 2D纹理
            TextureCube,  // 立方体贴图
            Texture3D,    // 3D纹理
            TextureArray  // 纹理数组
        } m_type = TextureType::Texture2D;

        bool m_needMipmap = true;// 是否需要生成mipmap
    };

    class Texture2D
    {
    public:
        Texture2D() = default;
        ~Texture2D() = default;

        // 禁用拷贝，允许移动
        Texture2D(const Texture2D&) = delete;
        Texture2D& operator=(const Texture2D&) = delete;
        Texture2D(Texture2D&&) noexcept = default;
        Texture2D& operator=(Texture2D&&) noexcept = default;

        // 获取纹理宽度
        [[nodiscard]] uint32_t GetWidth() const { return m_width; }

        // 获取纹理高度
        [[nodiscard]] uint32_t GetHeight() const { return m_height; }

        // 获取纹理格式
        [[nodiscard]] uint32_t GetChannels() const { return m_channels; }

        // 检查纹理是否有效
        [[nodiscard]] bool IsValid() const { return m_impl != nullptr; }
    private:
        explicit Texture2D(std::unique_ptr<VulkanTexture2D> impl,
                           uint32_t width,
                           uint32_t height,
                           uint32_t channels);

        std::unique_ptr<VulkanTexture2D> m_impl;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_channels = 0;
        uint32_t mipLevelCount = 1;
    };


} // TE

