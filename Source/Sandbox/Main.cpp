// ToyEngine Sandbox - Phase 1 测试应用
// 测试 Engine 主循环和 Math 模块

#include "Engine.h"

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
