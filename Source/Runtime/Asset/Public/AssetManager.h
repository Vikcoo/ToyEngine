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
    class AssetManager
{
public:
    AssetManager() = default;
    ~AssetManager();

    static AssetManager* GetInstance();

    template<typename T>
    static std::shared_ptr<T> Load(const std::string& AssetPath);

private:
    static AssetManager* m_instance;
};

template<>
std::shared_ptr<Mesh> AssetManager::Load<Mesh>(const std::string& AssetPath);

template<>
std::shared_ptr<RawTextureData> AssetManager::Load<RawTextureData>(const std::string& AssetPath);

} // TE