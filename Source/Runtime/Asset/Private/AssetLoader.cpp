//
// Created by yukai on 2026/1/3.
//

#include "AssetLoader.h"
#include "Loaders/ModelLoader.h"
#include "Log/Log.h"
#include "Mesh.h"
#include "Loaders/TextureLoader.h"

namespace TE
{
    template <>
    std::shared_ptr<Mesh> AssetLoader::Load<Mesh>(const std::string& AssetPath)
    {
        std::shared_ptr<Mesh> loadedMesh = ModelLoader::LoadFromFile(AssetPath);

        if (loadedMesh == nullptr || !loadedMesh->isValid())
        {
            TE_LOG_ERROR("Failed to load model");
        }
        return loadedMesh;
    }

    template <>
    std::shared_ptr<RawTextureData> AssetLoader::Load<RawTextureData>(const std::string& AssetPath)
    {
        std::shared_ptr<RawTextureData> loadedRawTextureData = TextureLoader::LoadFromFile(AssetPath);

        if (loadedRawTextureData == nullptr)
        {
            TE_LOG_ERROR("Failed to load model");
        }
        return loadedRawTextureData;
    }

} // TE