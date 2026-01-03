//
// Created by yukai on 2026/1/3.
//

#include "AssetManager.h"
#include "Loaders/ModelLoader.h"
#include "Log/Log.h"
#include "Mesh.h"

TE::AssetManager* TE::AssetManager::m_instance = nullptr;
namespace TE
{
    AssetManager::~AssetManager()
    {
        if (m_instance != nullptr)
        {
            delete m_instance;
        }
        m_instance = nullptr;
    }

    AssetManager* AssetManager::GetInstance()
    {
        if (m_instance == nullptr)
        {
            m_instance = new AssetManager();
        }
        return m_instance;
    }

    template <>
    std::shared_ptr<Mesh> AssetManager::Load<Mesh>(const std::string& AssetPath)
    {
        std::shared_ptr<Mesh> loadedMesh = ModelLoader::LoadFromFile(AssetPath);

        if (loadedMesh == nullptr || !loadedMesh->isValid())
        {
            TE_LOG_ERROR("Failed to load model");
        }
        return loadedMesh;
    }

} // TE