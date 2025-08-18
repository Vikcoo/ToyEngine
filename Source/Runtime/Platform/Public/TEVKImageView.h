//
// Created by yukai on 2025/8/18.
//
#pragma once
#include "TEVKCommon.h"
#include "TEVKLogicDevice.h"

namespace TE {
    class TEVKLogicDevice;

    class TEVKImageView {
        public:
        TEVKImageView(TEVKLogicDevice* logicDevice, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);
        ~TEVKImageView();
        vk::ImageView GetHandle(){ return  m_handle;}
    private:
        vk::ImageView m_handle = VK_NULL_HANDLE;
        TEVKLogicDevice* m_logicDevice;
    };
}


