// ToyEngine RenderCore Module
// 游戏线程向渲染线程提交的场景变更命令

#pragma once

#include "LightComponentId.h"
#include "LightSceneProxy.h"
#include "Math/MathTypes.h"
#include "PrimitiveComponentId.h"
#include "PrimitiveSceneProxy.h"

#include <memory>
#include <variant>

namespace TE {

/** 向渲染场景添加 Primitive，并移交 SceneProxy 所有权。 */
struct FAddPrimitiveCommand
{
    FPrimitiveComponentId PrimitiveId;
    std::unique_ptr<FPrimitiveSceneProxy> Proxy;
};

/** 更新渲染侧 Primitive 的世界变换。 */
struct FUpdatePrimitiveTransformCommand
{
    FPrimitiveComponentId PrimitiveId;
    Matrix4 WorldMatrix = Matrix4::Identity;
};

/** 从渲染场景移除 Primitive。 */
struct FRemovePrimitiveCommand
{
    FPrimitiveComponentId PrimitiveId;
};

/** 向渲染场景添加 Light，并移交 SceneProxy 所有权。 */
struct FAddLightCommand
{
    FLightComponentId LightId;
    std::unique_ptr<FLightSceneProxy> Proxy;
};

/** 替换渲染侧 Light 的状态快照。 */
struct FUpdateLightCommand
{
    FLightComponentId LightId;
    std::unique_ptr<FLightSceneProxy> Proxy;
};

/** 从渲染场景移除 Light。 */
struct FRemoveLightCommand
{
    FLightComponentId LightId;
};

/** 游戏线程构造、渲染线程消费的单条场景变更命令。 */
using FRenderSceneCommand = std::variant<
    FAddPrimitiveCommand,
    FUpdatePrimitiveTransformCommand,
    FRemovePrimitiveCommand,
    FAddLightCommand,
    FUpdateLightCommand,
    FRemoveLightCommand>;

} // namespace TE