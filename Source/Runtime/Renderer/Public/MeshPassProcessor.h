// ToyEngine Renderer Module
// FMeshPassProcessor - 最小 MeshPass 命令构建器

#pragma once

#include "MeshDrawCommand.h"

#include <vector>

namespace TE {

class FScene;

class FMeshPassProcessor
{
public:
    explicit FMeshPassProcessor(EMeshPassType passType);

    void BuildDrawCommands(const FScene* scene, std::vector<FMeshDrawCommand>& outCommands) const;

private:
    EMeshPassType m_PassType = EMeshPassType::BasePass;
};

} // namespace TE
