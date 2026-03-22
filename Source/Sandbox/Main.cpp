// ToyEngine Sandbox - Phase 1 测试应用
// 测试 Engine 主循环和 Math 模块

#include "Engine.h"

// Math 模块测试（可选，取消注释以启用）
// #include "Math/MathTypes.h"
// #include "Math/Transform.h"
// #include "Math/MathUtils.h"
// #include "Math/Color.h"
// #include "Math/Random.h"
// #include "Math/Geometry.h"

int main()
{
    // 获取引擎实例并初始化
    TE::Engine& engine = TE::Engine::Get();
    engine.Init();

    // 运行主循环（阻塞直到退出）
    engine.Run();

    // 关闭引擎
    engine.Shutdown();

    return 0;
}
