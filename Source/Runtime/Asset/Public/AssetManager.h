//
// Created by yukai on 2026/1/3.
//

#pragma once
#include <memory>
#include <string>

namespace TE
{
    class Mesh;
    class AssetManager
{
public:
    AssetManager() = default;
    ~AssetManager();

    static AssetManager* GetInstance();

    template<typename T>
    std::shared_ptr<T> Load(const std::string& AssetPath);

private:
    static AssetManager* m_instance;
};

template<>
std::shared_ptr<Mesh> AssetManager::Load<Mesh>(const std::string& AssetPath);

} // TE