# 临时问题分析：MathTest 旋转约定

本文记录 `MathTest` 曾经暴露的旋转约定问题，以及本次修复后的实现约定和验证结果。

## 现象

Release 构建下运行 `MathTest.exe` 时，以下用例失败：
- `Quat Euler angles conversion`
- `Transform LookAt GetForward`

对应测试位于 `Tests/MathTest.cpp`：
- `Quat::FromEuler(0, PI / 2, 0)` 后调用 `ToEulerAngles()`，期望返回的 Pitch 仍为 `PI / 2`。
- `Transform::LookAtRH((0,0,5), (0,0,0), Up)` 后调用 `GetForward()`，期望前向为 `(0,0,-1)`。

## 当前代码路径

相关实现集中在：
- `Source/Runtime/Core/Private/Math/MathTypes.cpp`
- `Source/Runtime/Core/Private/Math/Transform.cpp`
- `Source/Runtime/Core/Public/Math/MathTypes.h`
- `Source/Runtime/Core/Public/Math/Transform.h`

## 修复后状态

当前已完成修复：
- `Quat::ToEulerAngles()` 改为按 `Quat::FromEuler()` 的同一约定反解：`yaw(Y) * pitch(X) * roll(Z)`。
- `Transform::SetForwardRH()`、成员 `LookAtRH()`、静态 `LookAtRH()` 共用同一个右手系前向构造辅助逻辑。
- 普通 `Transform` 明确保持局部 `+Z` 为前向，局部 `+X` 为右向，局部 `+Y` 为上向。
- 构造 LookAt 正交基时使用 `right = normalize(cross(up, forward))`，再用 `correctedUp = normalize(cross(forward, right))`，避免只翻转一个轴导致反射矩阵。
- `CameraComponent` 仍保持相机局部 `-Z` 为看向前方的特殊约定，该差异继续限制在相机组件内部。
- `MathTest` 增加了 yaw/roll 往返、`LookAtRH +X` 基向量和 `SetForwardRH -Z` 基向量测试。
- 矩阵 View/Projection API 已改为显式命名：`LookAtRH/LH`、`PerspectiveRH/LH_ZO/NO`、`OrthographicRH/LH_ZO`，当前相机默认使用右手系 `[0, 1]` 深度范围。

验证结果：
- Release 构建通过。
- 补齐 CLion MinGW `PATH` 后，在沙箱外运行 `cmake-build-release/bin/MathTest.exe`，所有测试通过。

需要继续注意：
- 欧拉角在万向节锁附近仍不是无损表达；当前实现选择在锁定时固定 `roll = 0`，保留等价朝向的 `yaw`。
- 本次只修复普通 `Transform` 与当前测试覆盖到的旋转约定，没有扩展到动画、编辑器 gizmo 或序列化格式。

修复前，`Quat::FromEuler(float yaw, float pitch, float roll)` 的注释和实现表达的是：
- `yaw` 绕 Y 轴
- `pitch` 绕 X 轴
- `roll` 绕 Z 轴
- 最终组合顺序为 `yawQuat * pitchQuat * rollQuat`

但修复前 `Quat::ToEulerAngles()` 把结果写回 `Vector3 angles` 时，并没有严格按上述 Y/X/Z 构造路径求逆。代码中的 `sinp` 使用 `q.W * q.Y - q.Z * q.X`，并把结果写入 `angles.Y`；后续 `angles.X` 和 `angles.Z` 的公式也更接近另一套欧拉角分解约定。

这会导致 `FromEuler()` 与 `ToEulerAngles()` 不是稳定的往返关系。测试不是单纯精度误差，而是在暴露“参数命名、轴语义、旋转顺序、返回分量含义”没有形成闭环。当前已通过矩阵分解方式让 `ToEulerAngles()` 对齐 `yaw(Y) * pitch(X) * roll(Z)`。

## LookAt 与前向轴的问题

`Transform::GetForward()` 当前返回：

```cpp
return Rotation * Vector3::Forward;
```

注释明确表示项目在普通 `Transform` 层把局部 Z 轴作为前向。

修复前，旧 `Transform::LookAt()` 用如下思路构造正交基：
- `forward = normalize(target - Position)`
- `right = normalize(cross(forward, worldUp))`
- `up = cross(right, forward)`
- 将 `right / up / forward` 写入旋转矩阵
- 再通过 `Transform::FromMatrix(rotMatrix).Rotation` 提取四元数

失败说明当时至少有一处约定没有对齐：
- `Vector3::Forward` 到底是局部 `+Z` 还是局部 `-Z`
- `LookAtRH()` 构造的矩阵中，`right/up/forward` 被写入的是行还是列
- `Transform::FromMatrix()` 按哪种矩阵存储约定提取四元数
- `cross(forward, up)` 与 `cross(up, forward)` 对右手系/左手系的含义是否一致

当前 `CameraComponent` 又有一层特殊约定：注释写明“相机局部前向约定为 -Z”，并在多处使用 `-GetForward()`。普通物体 Transform 与相机 View 语义允许分叉，但必须明确写成规则，不能让 `Transform::LookAtRH()` 和 `CameraComponent::LookAt()` 隐式混用。

## 根因判断

该问题的核心不是某一个公式写错，而是旋转系统曾经缺少统一规格：
- 没有明确项目级坐标系约定：右手系还是左手系。
- 没有明确普通对象的局部前向轴：`+Z` 还是 `-Z`。
- 没有明确相机对象是否采用特殊前向轴，以及特殊性应该停留在 `CameraComponent` 还是进入 `Transform`。
- 没有明确欧拉角接口的参数语义：`yaw/pitch/roll` 分别映射到哪些轴，以及组合顺序是内旋还是外旋。
- `FromEuler()`、`ToEulerAngles()`、`LookAtRH()`、`GetForward()`、`FromMatrix()` 分别按局部假设实现，缺少统一测试矩阵覆盖。

## 影响范围

短期影响：
- `MathTest` 不能作为绿色基线。
- 相机朝向、光源方向、调试模型朝向出现问题时，很难判断是数学层、World 层还是 Renderer 层问题。

长期影响：
- 编辑器 gizmo、动画、骨骼、物理、序列化一旦依赖旋转语义，后续改约定会引起大面积返工。
- 多后端渲染时，投影矩阵和 View 矩阵差异会进一步放大坐标系混乱。

## 本次修复采用的方向

本次按以下方向落地：
1. 明确 `Vector3::Forward` 为普通对象局部 `+Z`。
2. 明确普通 `Transform` 的局部基向量：Right / Up / Forward 分别对应局部 `+X / +Y / +Z`。
3. 相机保持局部 `-Z` 看向前方，差异限制在 `CameraComponent` 和 ViewMatrix 构造中。
4. `Quat::FromEuler()` 保持 Y/X/Z 顺序，`ToEulerAngles()` 使用对应的矩阵逆分解。
5. 修正并显式命名 `Transform::LookAtRH()` 与 `SetForwardRH()` 的正交基构造，使 `GetForward()` 与目标方向一致。
6. 增加测试覆盖：
   - yaw 90、pitch 90、roll 90 的单轴旋转
   - `FromEuler -> ToEulerAngles` 的可逆区间
   - yaw/roll 的欧拉角往返
   - `LookAtRH` 看向 `+X`
   - `SetForwardRH` 指向 `-Z`

后续仍可继续扩展：
- yaw 90、pitch 90、roll 90 的更多单轴旋转组合
- `LookAtRH` 看向 `-X / +Y / -Y / +Z / -Z`
- 相机 `LookAt` 与 `BuildViewInfo` 的 ViewMatrix 方向一致性

## 后续排查优先级

如果后续遇到“方向反了”“相机移动反了”“光照方向不对”，优先检查：
1. 当前代码使用的是普通 `Transform::GetForward()` 还是相机语义的 `-GetForward()`。
2. 构造旋转时使用的是 `Quat::FromEuler()`、`Transform::LookAtRH()` 还是 `CameraComponent::LookAt()`。
3. 矩阵传入 shader 前是否经历了 ViewMatrix 或投影矩阵坐标范围调整。
4. 测试是否已经覆盖当前轴向和旋转顺序。
