//
// Created by yukai on 2026/1/3.
//

#pragma once
#include <memory>
#include <string>

namespace TE
{
    struct RawTextureData;
    class Texture2D;
    class TextureLoader
    {
    public:
        static std::shared_ptr<RawTextureData> LoadFromFile(const std::string& filePath);
    };
} // TE