# 数学坐标系与矩阵 API 约定

本文记录 `Core/Math` 当前已经落地的坐标系、局部轴、矩阵封装和命名规则。数学 API 属于公共基础设施，调用侧不能依赖“默认 LookAt / Perspective”这类隐式约定。

## 引擎默认空间约定

- 世界坐标采用右手系，`+Y` 为上。
- 普通 `Transform` 的局部轴约定为：`+X` 右、`+Y` 上、`+Z` 前。
- `CameraComponent` 是特例：相机局部 `-Z` 为看向前方。该特例限制在相机组件内部，普通 `Transform` 不跟随相机语义。
- `Matrix3` / `Matrix4` 按列主序存储，与 GLM 和当前 shader 上传约定一致。
- 引擎规范投影深度范围为 `[0, 1]`。OpenGL 后端当前通过 `glClipControl` 对齐该范围。

## 命名规则

涉及坐标系手性的矩阵 API 必须在名称中写明 `RH` 或 `LH`：

- `RH`：Right-Handed，右手系。
- `LH`：Left-Handed，左手系。

涉及 NDC 深度范围的投影 API 必须在名称中写明范围：

- `ZO`：Zero To One，深度范围 `[0, 1]`。
- `NO`：Negative One To One，深度范围 `[-1, 1]`。

因此，不再提供无后缀的 `Matrix4::LookAt()`、`Matrix4::Perspective()`、`Matrix4::Orthographic()` 作为公共入口。调用者必须显式选择手性和深度范围。

## 当前矩阵 API

- `Matrix4::LookAtRH()`：右手系 View 矩阵。
- `Matrix4::LookAtLH()`：左手系 View 矩阵。
- `Matrix4::PerspectiveRH_ZO()`：右手系透视投影，深度 `[0, 1]`。这是当前相机默认使用的投影。
- `Matrix4::PerspectiveLH_ZO()`：左手系透视投影，深度 `[0, 1]`。
- `Matrix4::PerspectiveRH_NO()`：右手系透视投影，深度 `[-1, 1]`。仅在需要 OpenGL 原生 NDC 或专项验证时使用。
- `Matrix4::PerspectiveLH_NO()`：左手系透视投影，深度 `[-1, 1]`。
- `Matrix4::OrthographicRH_ZO()`：右手系正交投影，深度 `[0, 1]`。
- `Matrix4::OrthographicLH_ZO()`：左手系正交投影，深度 `[0, 1]`。

## Transform 朝向 API

普通 `Transform` 的朝向构造目前只落地右手系版本：

- `Transform::SetForwardRH()`：按右手正交基设置局部 `+Z` 前向。
- `Transform::LookAtRH()`：按右手正交基让局部 `+Z` 指向目标方向。

相机需要“看向目标”时继续调用 `CameraComponent::LookAt()`。它内部使用 `Matrix4::LookAtRH()` 反解相机世界旋转，并保持相机局部 `-Z` 前向，不应改用普通 `Transform::LookAtRH()`。

`Transform::RotateWorld*()` 使用世界标准轴左乘旋转；`Transform::RotateLocal*()` 使用局部标准轴右乘旋转。两类组合完成后都会归一化四元数，避免连续旋转积累长度误差。

## Frustum 约定

当前 `Frustum` 只提供 `Frustum::FromViewProjectionRH_ZO()`：

- 输入必须是 `ProjectionRH_ZO * ViewRH`，即列向量约定下的 `Projection * View`。
- 输入必须使用右手系和 `[0, 1]` NDC 深度范围。
- Near 平面按 ZO 裁剪条件 `z >= 0` 从矩阵 `row2` 提取；不能复用 `[-1, 1]` 深度范围的 `row3 + row2` 公式。

需要支持 `LH` 或 `NO` 时，必须新增具有对应后缀的独立入口和测试，不能把不同规则隐藏在同一个函数中。

## 公共头文件分层

- `Math/Vector.h`：`Vector2`、`Vector3`、`Vector4`。
- `Math/Matrix.h`：`Matrix3`、`Matrix4`。
- `Math/Quat.h`：`Quat`。
- `Math/MathTypes.h`：上述三个头的兼容聚合入口；已有调用可以继续使用，新代码应按实际依赖包含具体头文件。

`Transform.h` 组合依赖 Vector、Matrix、Quat；`Geometry.h`、`MathUtils.h`、`Color.h`、`Random.h` 和 `VectorInt.h` 已只包含 `Vector.h`。

## 排查建议

如果出现相机方向、光照方向、剔除方向或深度测试异常，优先检查：

1. 调用的是 `RH` 还是 `LH` 版本。
2. 投影矩阵使用的是 `ZO` 还是 `NO` 深度范围。
3. 当前代码使用的是普通 `Transform` 的 `+Z` 前向，还是相机的 `-Z` 前向。
4. 后端是否在 RHI 层对 NDC Y、深度范围或正面缠绕做了额外适配。
