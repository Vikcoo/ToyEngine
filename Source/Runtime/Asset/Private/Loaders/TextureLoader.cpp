//
// Created by yukai on 2026/1/3.
//
#define STB_IMAGE_IMPLEMENTATION
#include "stb-master/stb_image.h"
#include "TextureLoader.h"
#include "Texture2D.h"
#include "Log/Log.h"

namespace TE
{
    std::shared_ptr<RawTextureData> TextureLoader::LoadFromFile(const std::string& filePath)
    {
        auto rawData = std::make_shared<RawTextureData>();
        int width, height, channels;

        // 读取图片文件，强制转换为4通道（RGBA），返回CPU端像素数据
        uint8_t* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels) {
            TE_LOG_ERROR("create texture2D failed");
            return rawData;
        }

        // 填充通用纹理数据
        rawData->m_width = static_cast<uint32_t>(width);
        rawData->m_height = static_cast<uint32_t>(height);
        rawData->m_channelCount = 4; // 强制RGBA
        rawData->m_dataSize = width * height * 4;
        rawData->m_pixelData.assign(pixels, pixels + rawData->m_dataSize);
        rawData->m_type = RawTextureData::TextureType::Texture2D;
        rawData->m_needMipmap = true;

        // 释放stb_image的临时内存
        stbi_image_free(pixels);
        return rawData;
    }
} // TE