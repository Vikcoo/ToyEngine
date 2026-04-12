// ToyEngine Sandbox
// 示例应用层：负责场景搭建与示例每帧逻辑

#include "Engine.h"

#include "Actor.h"
#include "CameraComponent.h"
#include "AssetImporter.h"
#include "FlyCameraController.h"
#include "Log/Log.h"
#include "Math/MathTypes.h"
#include "Math/ScalarMath.h"
#include "MeshComponent.h"
#include "PrimitiveComponent.h"
#include "StaticMesh.h"
#include "Window.h"
#include "World.h"

#include <memory>
#include <string>
#include <vector>
#include <filesystem>

namespace {

std::shared_ptr<TE::StaticMesh> LoadOrCreateDemoMesh(const std::string& modelName)
{
    const std::string modelDir = std::string(TE_PROJECT_ROOT_DIR) + "Content/Models/";
    const std::string meshPath = modelDir + modelName;

    if (auto loaded = TE::FAssetImporter::ImportStaticMesh(meshPath))
    {
        TE_LOG_INFO("[Sandbox] Loaded model from: {}", modelName);
        return loaded;
    }

    TE_LOG_INFO("[Sandbox] No external model found, creating default cube mesh");

    auto mesh = std::make_shared<TE::StaticMesh>();
    mesh->SetName("DefaultCube");

    TE::FMeshSection section;

    // 立方体 24 个顶点（每面 4 个独立顶点，每面不同颜色）
    // 前面 (Z+) - 红色
    section.Vertices.push_back({{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}, {0.9f, 0.2f, 0.2f}});
    section.Vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}, {0.9f, 0.2f, 0.2f}});
    section.Vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}, {0.9f, 0.2f, 0.2f}});
    section.Vertices.push_back({{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}, {0.9f, 0.2f, 0.2f}});

    // 后面 (Z-) - 绿色
    section.Vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}, {0.2f, 0.9f, 0.2f}});
    section.Vertices.push_back({{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}, {0.2f, 0.9f, 0.2f}});
    section.Vertices.push_back({{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}, {0.2f, 0.9f, 0.2f}});
    section.Vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}, {0.2f, 0.9f, 0.2f}});

    // 右面 (X+) - 蓝色
    section.Vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.2f, 0.2f, 0.9f}});
    section.Vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.2f, 0.2f, 0.9f}});
    section.Vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.2f, 0.2f, 0.9f}});
    section.Vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.2f, 0.2f, 0.9f}});

    // 左面 (X-) - 黄色
    section.Vertices.push_back({{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.9f, 0.9f, 0.2f}});
    section.Vertices.push_back({{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.9f, 0.9f, 0.2f}});
    section.Vertices.push_back({{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.9f, 0.9f, 0.2f}});
    section.Vertices.push_back({{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.9f, 0.9f, 0.2f}});

    // 顶面 (Y+) - 青色
    section.Vertices.push_back({{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {0.2f, 0.9f, 0.9f}});
    section.Vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}, {0.2f, 0.9f, 0.9f}});
    section.Vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}, {0.2f, 0.9f, 0.9f}});
    section.Vertices.push_back({{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}, {0.2f, 0.9f, 0.9f}});

    // 底面 (Y-) - 品红
    section.Vertices.push_back({{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}, {0.9f, 0.2f, 0.9f}});
    section.Vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}, {0.9f, 0.2f, 0.9f}});
    section.Vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}, {0.9f, 0.2f, 0.9f}});
    section.Vertices.push_back({{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}, {0.9f, 0.2f, 0.9f}});

    // 36 个索引（6 面 × 2 三角形 × 3 顶点，CCW 正面）
    section.Indices = {
         0,  1,  2,   2,  3,  0,    // 前面
         4,  5,  6,   6,  7,  4,    // 后面
         8,  9, 10,  10, 11,  8,    // 右面
        12, 13, 14,  14, 15, 12,    // 左面
        16, 17, 18,  18, 19, 16,    // 顶面
        20, 21, 22,  22, 23, 20,    // 底面
    };

    mesh->AddSection(std::move(section));
    return mesh;
}

void OverrideMeshBaseColorTexture(const std::shared_ptr<TE::StaticMesh>& mesh, const std::string& texturePath)
{
    if (!mesh)
    {
        return;
    }

    auto materials = mesh->GetMaterials();
    if (materials.empty())
    {
        materials.resize(1);
    }

    for (auto& material : materials)
    {
        material.BaseColorTexturePath = texturePath;
    }
    mesh->SetMaterials(std::move(materials));
}

void SetupSandboxScene(TE::Engine& engine)
{
    auto* world = engine.GetWorld();
    if (!world)
    {
        TE_LOG_ERROR("[Sandbox] World is null in scene setup");
        return;
    }

    auto loadedMesh = LoadOrCreateDemoMesh("orientation_cube.obj");
    auto loadedMeshTop = std::make_shared<TE::StaticMesh>(*loadedMesh);

    const std::string textureDir = std::string(TE_PROJECT_ROOT_DIR) + "Content/Textures/";
    const std::string textureA = textureDir + "orientation_cube_atlas.png";
    const std::string textureB = textureDir + "orientation_cube_atlas.png";

    if (std::filesystem::exists(textureA))
    {
        OverrideMeshBaseColorTexture(loadedMesh, textureA);
        TE_LOG_INFO("[Sandbox] MeshActor texture: {}", textureA);
    }
    else
    {
        TE_LOG_WARN("[Sandbox] Texture file not found: {}", textureA);
    }

    if (std::filesystem::exists(textureB))
    {
        OverrideMeshBaseColorTexture(loadedMeshTop, textureB);
        TE_LOG_INFO("[Sandbox] MeshActorTop texture: {}", textureB);
    }
    else
    {
        TE_LOG_WARN("[Sandbox] Texture file not found: {}", textureB);
    }

    auto meshActor = std::make_unique<TE::Actor>();
    meshActor->SetName("MeshActor");
    auto* meshComp = meshActor->AddComponent<TE::MeshComponent>();
    meshComp->SetName("ModelMesh");
    meshComp->SetStaticMesh(loadedMesh);

    auto meshActorTop = std::make_unique<TE::Actor>();
    meshActorTop->SetName("MeshActorTop");
    auto* meshCompTop = meshActorTop->AddComponent<TE::MeshComponent>();
    meshCompTop->SetName("ModelMeshTop");
    meshCompTop->SetStaticMesh(loadedMeshTop);
    meshCompTop->SetScale(TE::Vector3(0.5f, 0.5f, 0.5f));
    meshCompTop->SetPosition(TE::Vector3(0.0f, 1.5f, 0.0f));

    auto cameraActor = std::make_unique<TE::Actor>();
    cameraActor->SetName("CameraActor");
    auto* cameraComp = cameraActor->AddComponent<TE::CameraComponent>();
    cameraComp->SetName("MainCamera");
    cameraComp->SetFOV(60.0f);
    cameraComp->SetNearPlane(0.1f);
    cameraComp->SetFarPlane(100.0f);

    if (auto* window = engine.GetWindow())
    {
        cameraComp->SetViewportSize(
            static_cast<float>(window->GetFramebufferWidth()),
            static_cast<float>(window->GetFramebufferHeight())
        );
    }

    cameraComp->SetPosition(TE::Vector3(0.0f, 0.0f, 6.0f));
    cameraComp->LookAt(TE::Vector3::Zero);

    auto* flyCamCtrl = cameraActor->AddComponent<TE::FlyCameraController>();
    flyCamCtrl->SetName("FlyCameraController");
    flyCamCtrl->SetInputManager(engine.GetInputManager());
    flyCamCtrl->SetWindow(engine.GetWindow());

    world->AddActor(std::move(meshActor));
    world->AddActor(std::move(meshActorTop));
    world->AddActor(std::move(cameraActor));

    engine.SetActiveCameraComponent(cameraComp);
    TE_LOG_INFO("[Sandbox] Scene built: MeshActor + MeshActorTop + CameraActor");
}

void TickSandboxScene(TE::Engine& engine, float deltaTime)
{
    auto* world = engine.GetWorld();
    if (!world)
    {
        return;
    }

    const auto& actors = world->GetActors();
    for (const auto& actor : actors)
    {
        if (actor->GetName() == "MeshActor")
        {
            const float rotY = TE::Math::DegToRad(45.0f) * deltaTime;
            const float rotX = TE::Math::DegToRad(30.0f) * deltaTime;
            actor->GetTransform().RotateWorldY(rotY);
            actor->GetTransform().RotateWorldX(rotX);

            for (const auto& comp : actor->GetComponents())
            {
                if (auto* primComp = dynamic_cast<TE::PrimitiveComponent*>(comp.get()))
                {
                    primComp->MarkRenderStateDirty();
                }
            }
        }
    }
}

} // namespace

int main()
{
    TE::Engine& engine = TE::Engine::Get();
    engine.SetSceneSetupCallback(SetupSandboxScene);
    //engine.SetFrameUpdateCallback(TickSandboxScene);

    engine.Init();
    engine.Run();
    engine.Shutdown();
    return 0;
}
