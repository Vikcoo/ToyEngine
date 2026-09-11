// ToyEngine - 渲染场景命令收集器最小回归测试

#include "RenderSceneCommandRecorder.h"
#include "RendererScene.h"
#include "Actor.h"
#include "LightComponent.h"
#include "PrimitiveComponent.h"
#include "World.h"

#include <memory>
#include <utility>
#include <variant>
#include <vector>

namespace {

class FTestPrimitiveSceneProxy final : public TE::FPrimitiveSceneProxy
{
public:
    void GetMeshDrawCommands(std::vector<TE::FMeshDrawCommand>& outCommands) const override
    {
        (void)outCommands;
    }
};

class FTestPrimitiveComponent final : public TE::PrimitiveComponent
{
public:
    [[nodiscard]] std::unique_ptr<TE::FPrimitiveSceneProxy> CreateSceneProxy() const override
    {
        return std::make_unique<FTestPrimitiveSceneProxy>();
    }
};

} // namespace

int main()
{
    TE::FRenderSceneCommandRecorder recorder;

    if (recorder.AddPrimitive({}, std::make_unique<FTestPrimitiveSceneProxy>()) ||
        recorder.AddLight({}, std::make_unique<TE::FLightSceneProxy>()))
    {
        return 1;
    }
    recorder.UpdatePrimitiveTransform({}, TE::Matrix4::Identity);
    recorder.RemovePrimitive({});
    recorder.UpdateLight({}, std::make_unique<TE::FLightSceneProxy>());
    recorder.RemoveLight({});

    TE::FPrimitiveComponentId primitiveId;
    primitiveId.Value = 1;
    if (!recorder.AddPrimitive(primitiveId, std::make_unique<FTestPrimitiveSceneProxy>()))
    {
        return 1;
    }
    const TE::Matrix4 worldMatrix = TE::Matrix4::Translate({1.0f, 2.0f, 3.0f});
    recorder.UpdatePrimitiveTransform(primitiveId, worldMatrix);

    TE::FLightComponentId lightId;
    lightId.Value = 2;
    if (!recorder.AddLight(lightId, std::make_unique<TE::FLightSceneProxy>()))
    {
        return 1;
    }
    auto updatedLight = std::make_unique<TE::FLightSceneProxy>();
    updatedLight->Intensity = 3.0f;
    recorder.UpdateLight(lightId, std::move(updatedLight));

    auto commands = recorder.TakeCommands();
    const bool valid = commands.size() == 4 &&
                       std::holds_alternative<TE::FAddPrimitiveCommand>(commands[0]) &&
                       std::get<TE::FAddPrimitiveCommand>(commands[0]).Proxy != nullptr &&
                       std::holds_alternative<TE::FUpdatePrimitiveTransformCommand>(commands[1]) &&
                       std::holds_alternative<TE::FAddLightCommand>(commands[2]) &&
                       std::holds_alternative<TE::FUpdateLightCommand>(commands[3]) &&
                       recorder.TakeCommands().empty();
    if (!valid)
    {
        return 1;
    }

    TE::FScene scene(nullptr);
    scene.ApplyCommands(std::move(commands));
    if (scene.GetPrimitives().size() != 1 || scene.GetLights().size() != 1)
    {
        return 1;
    }

    const TE::Vector3 primitivePosition = scene.GetPrimitives().front()->GetWorldMatrix().GetTranslation();
    if (primitivePosition != TE::Vector3(1.0f, 2.0f, 3.0f) ||
        scene.GetLights().front()->Intensity != 3.0f)
    {
        return 1;
    }

    recorder.RemovePrimitive(primitiveId);
    recorder.RemoveLight(lightId);
    scene.ApplyCommands(recorder.TakeCommands());
    if (!scene.GetPrimitives().empty() || !scene.GetLights().empty())
    {
        return 1;
    }

    TE::World world;
    world.SetRenderSceneCommandRecorder(&recorder);
    auto actor = std::make_unique<TE::Actor>();
    FTestPrimitiveComponent* const primitive = actor->AddComponent<FTestPrimitiveComponent>();
    TE::Actor* const ownedActor = world.AddActor(std::move(actor));

    commands = recorder.TakeCommands();
    if (!ownedActor || commands.size() != 1 ||
        !std::holds_alternative<TE::FAddPrimitiveCommand>(commands.front()))
    {
        return 1;
    }

    TE::DirectionalLightComponent* const light = ownedActor->AddComponent<TE::DirectionalLightComponent>();
    commands = recorder.TakeCommands();
    if (!light || commands.size() != 1 || !std::holds_alternative<TE::FAddLightCommand>(commands.front()))
    {
        return 1;
    }

    world.UnregisterComponent(light);
    commands = recorder.TakeCommands();
    if (ownedActor->GetComponents().size() != 2 || commands.size() != 1 ||
        !std::holds_alternative<TE::FRemoveLightCommand>(commands.front()))
    {
        return 1;
    }
    world.RegisterComponent(light);
    commands = recorder.TakeCommands();
    if (commands.size() != 1 || !std::holds_alternative<TE::FAddLightCommand>(commands.front()))
    {
        return 1;
    }

    if (!ownedActor->RemoveComponent(primitive))
    {
        return 1;
    }
    commands = recorder.TakeCommands();
    if (commands.size() != 1 || !std::holds_alternative<TE::FRemovePrimitiveCommand>(commands.front()))
    {
        return 1;
    }

    if (!world.RemoveActor(ownedActor))
    {
        return 1;
    }
    commands = recorder.TakeCommands();
    if (commands.size() != 1 || !std::holds_alternative<TE::FRemoveLightCommand>(commands.front()))
    {
        return 1;
    }

    {
        TE::World scopedWorld;
        scopedWorld.SetRenderSceneCommandRecorder(&recorder);
        auto scopedActor = std::make_unique<TE::Actor>();
        FTestPrimitiveComponent* const scopedPrimitive = scopedActor->AddComponent<FTestPrimitiveComponent>();
        TE::Actor* const scopedOwnedActor = scopedWorld.AddActor(std::move(scopedActor));
        commands = recorder.TakeCommands();
        if (!scopedPrimitive || !scopedOwnedActor || commands.size() != 1 ||
            !std::holds_alternative<TE::FAddPrimitiveCommand>(commands.front()))
        {
            return 1;
        }
    }

    commands = recorder.TakeCommands();
    return commands.size() == 1 && std::holds_alternative<TE::FRemovePrimitiveCommand>(commands.front()) ? 0 : 1;
}
