/*
文件用途: 图像视图封装
- 负责: 为 vk::Image 创建对应的 vk::ImageView（决定像素格式/子资源范围/通道重映射等）
- 概念: ImageView 是使用 Image 的“方式”，如 2D 视图、Mipmap/Array 范围等
*/
#pragma once
#include "TEVKCommon.h"
#include "TEVKLogicDevice.h"

namespace TE {
    class TEVKLogicDevice;

    class TEVKImageView {
        public:
        TEVKImageView(TEVKLogicDevice& logicDevice, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);
        const vk::raii::ImageView& GetHandle(){ return m_handle;}
    private:
        vk::raii::ImageView m_handle{VK_NULL_HANDLE};
        TEVKLogicDevice& m_logicDevice;
    };
}