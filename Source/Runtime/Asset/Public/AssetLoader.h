//
// Created by yukai on 2026/1/3.
//

#pragma once
#include <memory>
#include <string>

namespace TE
{
    struct RawTextureData;
    class Mesh;
    class AssetLoader
{
public:
    AssetLoader() = default;
    ~AssetLoader() = default;

    static AssetLoader* GetInstance();

    template<typename T>
    static std::shared_ptr<T> Load(const std::string& AssetPath);
};

template<>
std::shared_ptr<Mesh> AssetLoader::Load<Mesh>(const std::string& AssetPath);

template<>
std::shared_ptr<RawTextureData> AssetLoader::Load<RawTextureData>(const std::string& AssetPath);

} // TE