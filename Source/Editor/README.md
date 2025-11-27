# Editor 模块

编辑器模块，用于构建可视化的游戏编辑器。

## 📋 状态

**当前状态：预留目录**

此目录为未来的编辑器开发预留，暂时保持为空。

## 🎯 未来规划

当Runtime模块成熟后，Editor将包含以下子模块：

### 建议的子模块结构

```
Editor/
├── EditorCore/          # 编辑器核心
│   ├── EditorApplication.h/cpp
│   ├── EditorWindow.h/cpp
│   └── EditorContext.h/cpp
│
├── LevelEditor/         # 关卡编辑器
│   ├── Viewport.h/cpp   # 3D视口
│   ├── Gizmo.h/cpp      # 变换工具
│   └── SceneHierarchy.h/cpp  # 场景层级面板
│
├── AssetEditor/         # 资产编辑器
│   ├── MaterialEditor/  # 材质编辑器
│   ├── MeshViewer/      # 模型查看器
│   └── TextureViewer/   # 纹理查看器
│
├── UIFramework/         # UI框架
│   ├── ImGuiLayer.h/cpp # ImGui集成
│   ├── Panels/          # 各种面板
│   └── Widgets/         # UI组件
│
└── Tools/               # 编辑器工具
    ├── AssetImporter/   # 资产导入工具
    └── ProjectGenerator/ # 项目生成器
```

## 📚 技术栈（规划）

- **ImGui** - 即时模式GUI库
- **ImGuizmo** - 3D Gizmo工具
- **NFD** - 原生文件对话框

## ⏰ 开发时机

建议在以下Runtime模块完成后开始Editor开发：

- ✅ Core模块
- ✅ Platform模块
- ✅ RHI模块
- ✅ Renderer模块
- ✅ Asset模块
- ✅ Scene模块（特别重要）
- ✅ 场景序列化系统

## 🎮 参考引擎

- **Unreal Engine 5** - 完整的编辑器架构
- **Unity** - 编辑器UI设计
- **Hazel Engine** - 轻量级编辑器实现
- **Godot** - 开源编辑器参考

## 💡 设计原则

1. **模块化** - Editor应该是可选的，不影响Runtime
2. **插件化** - 各种编辑器功能应该以插件形式存在
3. **可扩展** - 支持自定义编辑器窗口和工具

---

**注意**：在Runtime稳定之前，不建议开始Editor开发。专注于构建稳定的引擎核心。

**预计开发时间**：Runtime成熟后需要 3-6 个月

