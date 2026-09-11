// ToyEngine RenderCore Module
// 游戏线程侧的渲染场景命令收集器

#pragma once

#include "RenderSceneCommand.h"

#include <memory>
#include <utility>
#include <vector>

namespace TE {

/**
 * 收集当前帧的场景变更命令。
 * @note 该对象只允许游戏线程访问；跨线程同步由外层帧包队列负责。
 * @note Add 返回值只表示命令参数有效且已记录，不表示渲染侧资源已准备成功。
 */
class FRenderSceneCommandRecorder final
{
public:
    /** 记录 Primitive 添加命令并接管 Proxy。 */
    [[nodiscard]] bool AddPrimitive(FPrimitiveComponentId primitiveId,
                                    std::unique_ptr<FPrimitiveSceneProxy> proxy)
    {
        if (!primitiveId.IsValid() || !proxy)
        {
            return false;
        }

        m_Commands.emplace_back(FAddPrimitiveCommand{primitiveId, std::move(proxy)});
        return true;
    }

    /** 记录 Primitive 变换更新命令。 */
    void UpdatePrimitiveTransform(FPrimitiveComponentId primitiveId, const Matrix4& worldMatrix)
    {
        if (primitiveId.IsValid())
        {
            m_Commands.emplace_back(FUpdatePrimitiveTransformCommand{primitiveId, worldMatrix});
        }
    }

    /** 记录 Primitive 移除命令。 */
    void RemovePrimitive(FPrimitiveComponentId primitiveId)
    {
        if (primitiveId.IsValid())
        {
            m_Commands.emplace_back(FRemovePrimitiveCommand{primitiveId});
        }
    }

    /** 记录 Light 添加命令并接管 Proxy。 */
    [[nodiscard]] bool AddLight(FLightComponentId lightId, std::unique_ptr<FLightSceneProxy> proxy)
    {
        if (!lightId.IsValid() || !proxy)
        {
            return false;
        }

        m_Commands.emplace_back(FAddLightCommand{lightId, std::move(proxy)});
        return true;
    }

    /** 记录 Light 状态更新命令并接管 Proxy。 */
    void UpdateLight(FLightComponentId lightId, std::unique_ptr<FLightSceneProxy> proxy)
    {
        if (lightId.IsValid() && proxy)
        {
            m_Commands.emplace_back(FUpdateLightCommand{lightId, std::move(proxy)});
        }
    }

    /** 记录 Light 移除命令。 */
    void RemoveLight(FLightComponentId lightId)
    {
        if (lightId.IsValid())
        {
            m_Commands.emplace_back(FRemoveLightCommand{lightId});
        }
    }

    /** 取走当前已记录的全部命令，并开始收集下一批命令。 */
    [[nodiscard]] std::vector<FRenderSceneCommand> TakeCommands()
    {
        return std::exchange(m_Commands, std::vector<FRenderSceneCommand>{});
    }

private:
    std::vector<FRenderSceneCommand> m_Commands;
};

} // namespace TE
