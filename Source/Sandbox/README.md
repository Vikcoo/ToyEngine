# Sandbox

沙盒程序，用于测试和演示引擎功能。

## 📁 目录结构

- `Main.cpp` - 程序入口
- `ExampleLayer.cpp` - 示例关卡

## 📝 示例代码

```cpp
// Main.cpp
#include <ToyEngine.h>

class SandboxApp : public TE::Application {
public:
    SandboxApp() : Application("Sandbox") {}
    
    void OnInit() override {
        TE_LOG_INFO("Sandbox initialized!");
        
        // 加载资源
        m_Mesh = TE::AssetManager::Load<TE::Mesh>("models/cube.obj");
        m_Texture = TE::AssetManager::Load<TE::Texture2D>("textures/container.png");
        
        // 创建材质
        m_Material = std::make_shared<TE::Material>();
        m_Material->SetTexture("u_Texture", m_Texture);
    }
    
    void OnUpdate(float dt) override {
        // 更新逻辑
        m_Rotation += dt;
        
        // 相机控制
        if (TE::Input::IsKeyPressed(TE::KeyCode::W)) {
            m_Camera.MoveForward(2.0f * dt);
        }
    }
    
    void OnRender() override {
        // 清屏
        TE::RenderCommand::Clear(0.1f, 0.1f, 0.1f);
        
        // 开始场景
        TE::Renderer::BeginScene(m_Camera);
        
        // 提交渲染对象
        auto transform = glm::rotate(glm::mat4(1.0f), m_Rotation, glm::vec3(0, 1, 0));
        TE::Renderer::Submit(m_Mesh, m_Material, transform);
        
        // 结束场景
        TE::Renderer::EndScene();
    }
    
    void OnShutdown() override {
        TE_LOG_INFO("Sandbox shutdown!");
    }
    
private:
    TE::Camera m_Camera;
    std::shared_ptr<TE::Mesh> m_Mesh;
    std::shared_ptr<TE::Texture2D> m_Texture;
    std::shared_ptr<TE::Material> m_Material;
    float m_Rotation = 0.0f;
};

// 引擎入口点
TE_CREATE_APPLICATION(SandboxApp)
```

## 🎯 用途

1. **功能测试** - 测试引擎各个模块
2. **性能测试** - 压力测试和性能分析
3. **示例展示** - 演示引擎使用方法
4. **快速原型** - 快速验证想法

## ✅ 示例清单

- [ ] Hello Triangle（基础渲染）
- [ ] Textured Cube（纹理映射）
- [ ] Camera Controls（相机控制）
- [ ] Multiple Objects（多对象渲染）
- [ ] Lighting（光照系统）
- [ ] Scene Graph（场景图）

