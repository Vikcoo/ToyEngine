# 临时问题分析：SimpleTest 内存 Shutdown 并发

本文记录 `SimpleTest` 曾经暴露的内存系统 shutdown 并发问题，以及本次按方案 A 修复后的接口契约和验证结果。

## 现象

Release 构建下运行 `SimpleTest.exe` 时，测试在输出一次内存统计后以 `0xC0000005` 访问冲突退出，没有到达全部通过。

修复前，直接相关的测试是 `Tests/SimpleTest.cpp` 中的 `TestShutdownRaceNoCrash()`：
- 主线程先调用 `MemoryInit()`。
- 创建多个 worker 线程循环执行 `MemAlloc()`、写入返回指针、`MemFree()`。
- 另一个线程短暂 sleep 后调用 `MemoryShutdown()`。
- shutdown 后才通过 `stop` 通知 worker 停止。

也就是说，原测试刻意制造了 `MemoryShutdown()` 与 worker 分配/写入/释放并发发生的情况。按方案 A，这个测试语义本身不再合法，因此已改为有序停机测试。

## 当前代码路径

相关实现集中在：
- `Source/Runtime/Core/Private/Memory/Memory.cpp`
- `Source/Runtime/Core/Private/Memory/TlsfAllocator.cpp`
- `Source/Runtime/Core/Public/Memory/Memory.h`
- `Tests/SimpleTest.cpp`

## 修复后状态

当前已按方案 A 完成修复：
- `MemoryShutdown()` 明确为有序停机接口，不支持与 `MemAlloc()` / `MemFree()` 并发调用。
- `Memory.h` 已补充接口注释：调用 `MemoryShutdown()` 前，必须保证所有可能使用内存系统的 worker 已停止，且由本内存系统分配的裸指针都已经释放。
- `Tests/SimpleTest.cpp` 中原 `TestShutdownRaceNoCrash()` 已改为 `TestOrderedShutdownNoCrash()`。
- 测试不再制造 shutdown 与 worker 写入裸指针并发的非法场景，而是先通知 worker 停止、join 所有 worker，再调用 `MemoryShutdown()`。

验证结果：
- Release 构建通过。
- 补齐 CLion MinGW `PATH` 后，在沙箱外运行 `cmake-build-release/bin/SimpleTest.exe`，输出 `all passed`，退出码为 `0`。

仍然保留的限制：
- 当前实现不支持任意线程在 `MemoryShutdown()` 期间继续分配、写入或释放内存。
- 如果未来需要热重载、可重入内存域或强并发 shutdown，应作为方案 B 的独立架构任务处理。
- `MemFree(ptr)` 仍然只面向当前全局 allocator；方案 A 依赖调用方遵守“关闭前释放完旧指针”的生命周期契约。

`Memory.cpp` 当前维护一个全局状态：

```cpp
std::mutex g_memoryMutex;
std::shared_ptr<TlsfAllocator> g_allocator;
```

`MemAlloc()` 的路径是：
1. `GetOrCreateAllocator()` 在锁内取得或创建 `g_allocator`。
2. 返回一个 `std::shared_ptr<TlsfAllocator>` 快照。
3. 调用 `alloc->Allocate(...)`。
4. 向调用方返回裸指针。

`MemFree()` 的路径是：
1. `GetAllocatorSnapshot()` 读取当前 `g_allocator`。
2. 如果当前没有 allocator，直接返回。
3. 否则调用当前 allocator 的 `Free(ptr)`。

`MemoryShutdown()` 的路径是：
1. 创建局部 `std::shared_ptr<TlsfAllocator> old`。
2. 在锁内执行 `old.swap(g_allocator)`。
3. 函数返回时 `old` 析构，可能销毁旧 allocator 及其 TLSF pool。

## 根因判断

这里的关键问题是：`std::shared_ptr<TlsfAllocator>` 只保护了 allocator 对象在 `MemAlloc()` 函数调用期间存活，并不保护调用方拿到的裸指针在后续写入和释放期间仍然有效。

一个可能的竞态序列如下：
1. worker 调用 `MemAlloc()`。
2. `MemAlloc()` 内部拿到 allocator 快照，并从 TLSF pool 返回裸指针 `p`。
3. `MemAlloc()` 返回，局部 `shared_ptr` 释放。
4. shutdown 线程调用 `MemoryShutdown()`，把 `g_allocator` 置空，并销毁旧 allocator。
5. 旧 allocator 析构释放底层 pool。
6. worker 继续执行 `*static_cast<std::uint64_t*>(p) = local` 或 `MemFree(p)`。
7. `p` 指向的内存已经被 OS 释放或不再属于有效 pool，于是触发访问冲突或无效释放。

因此，当前 crash 不是简单的 TLSF 内部分配锁失效，而是全局 allocator 生命周期与已分配裸指针生命周期之间没有建立 shutdown 协议。

## 为什么 `shared_ptr` 不足以解决

当前 `shared_ptr` 的保护范围是 allocator 对象，不是每一块分配出去的内存。

只要 API 仍然返回裸指针，就会出现以下断点：
- `MemAlloc()` 返回后，调用方不再持有 allocator 的所有权。
- `MemoryShutdown()` 无法知道还有多少裸指针正在被使用。
- `MemFree(ptr)` 只看“当前全局 allocator”，无法知道 `ptr` 属于哪个历史 allocator。
- 如果 shutdown 后又有线程懒创建了新 allocator，旧指针甚至可能被交给新 allocator 释放，形成更隐蔽的问题。

所以 `g_allocator` 使用 `shared_ptr` 只能避免 allocator 在 `Allocate()` 函数内部被销毁，不能保证分配结果跨函数调用安全。

## 修复前影响范围

修复前的短期影响：
- `SimpleTest` 不能作为绿色基线。
- 只要 shutdown 与 worker 未完全同步，测试和引擎退出都可能出现访问冲突。

修复前的长期影响：
- 后续任务系统、资源异步加载、渲染线程、热重载或测试隔离都会依赖明确的 shutdown 边界。
- 如果内存系统允许“shutdown 后懒创建新 allocator”，旧指针归属会变得更难判断。
- 全局 `new/delete` 已接入 `MemAlloc/MemFree`，因此 STL 或第三方对象析构时也可能间接受影响。

## 可选修复策略

需要先决定内存系统的契约。至少有两种方向：

### 方案 A：严格外部停机契约

规定 `MemoryShutdown()` 只能在所有 worker 停止、所有引擎对象释放之后调用。

优点：
- 实现简单。
- 符合很多引擎的关闭流程：先停任务系统，再释放资源，最后关闭内存系统。

代价：
- `TestShutdownRaceNoCrash()` 的测试目标需要调整，不能要求 allocator 支持任意并发 shutdown。
- 文档和断言必须明确说明 shutdown 不是线程抢占安全接口。

### 方案 B：allocator 内部支持并发 shutdown

让内存系统在实现上支持 shutdown 与分配/释放并发。

可能需要：
- 全局关闭状态，shutdown 开始后禁止新分配或返回 null。
- 活跃操作计数，shutdown 等待所有 `Allocate/Free/Reallocate` 完成。
- 已分配块的 allocator/pool 归属信息，保证 `MemFree(ptr)` 能找到原始 allocator。
- 延迟释放旧 pool，直到所有可能持有旧裸指针的使用者退出。

优点：
- 接口更健壮。
- 测试语义可以保持“shutdown race no crash”。

代价：
- 实现复杂，容易把全局分配器做成半个生命周期管理系统。
- 单靠 allocator 很难知道调用方是否还会写入裸指针；即使等待 `MemFree()` 操作，也无法阻止 `MemAlloc()` 返回后、`MemFree()` 前的普通写入。

## 本次采用的修复方案

本次采用方案 A：
- 明确 `MemoryShutdown()` 的调用前置条件：所有使用 `MemAlloc/MemFree` 的 worker 必须已经停止。
- 将 `TestShutdownRaceNoCrash()` 改为 `TestOrderedShutdownNoCrash()`，测试“停机顺序正确时不崩溃”，而不是要求裸指针在异步 shutdown 后仍安全。
- 在 `Memory.h` 中写明 `MemoryShutdown()` 不支持与分配/释放并发调用。

如果未来确实需要热重载或可重入内存域，再考虑方案 B，但那应作为单独架构任务处理，而不是只在当前全局 allocator 上补一个锁。

## 后续排查优先级

如果后续遇到内存 shutdown 相关崩溃，优先检查：
1. 是否还有 worker、资源加载线程、渲染线程或测试线程在 shutdown 后继续调用 `MemAlloc/MemFree`。
2. `MemoryShutdown()` 是否早于引擎对象、STL 容器、全局对象析构。
3. `MemFree(ptr)` 释放的指针是否来自当前 allocator，还是来自 shutdown 前的旧 allocator。
4. crash 是否发生在写入 `MemAlloc()` 返回指针之后、`MemFree()` 之前。
