// ToyEngine Renderer Module
// FMeshPassProcessor 实现

#include "MeshPassProcessor.h"

#include "PrimitiveSceneProxy.h"
#include "RendererScene.h"

namespace TE {

FMeshPassProcessor::FMeshPassProcessor(EMeshPassType passType)
    : m_PassType(passType)
{
}

void FMeshPassProcessor::BuildDrawCommands(const FScene* scene, std::vector<FMeshDrawCommand>& outCommands) const
{
    if (!scene)
    {
        return;
    }

    const auto& primitives = scene->GetPrimitives();
    outCommands.reserve(primitives.size() * 2);

    for (const auto* proxy : primitives)
    {
        if (!proxy)
        {
            continue;
        }

        const auto commandStart = outCommands.size();
        proxy->GetMeshDrawCommands(outCommands);

        for (auto index = commandStart; index < outCommands.size(); ++index)
        {
            outCommands[index].PipelineKey.Pass = m_PassType;
        }
    }
}

} // namespace TE
