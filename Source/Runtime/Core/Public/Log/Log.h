// ToyEngine Core Module
// 日志系统 - 基于spdlog的封装

#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include "spdlog/spdlog.h"
#include "spdlog/common.h"

namespace TE {

/// <summary>
/// 日志管理器 - 封装spdlog，提供统一的日志接口
/// </summary>
class Log {
public:
    // 禁用拷贝和赋值
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

    /// <summary>
    /// 初始化日志系统（必须在使用日志前调用）
    /// </summary>
    static void Init();

    /// <summary>
    /// 获取日志实例
    /// </summary>
    static Log& GetInstance();

    /// <summary>
    /// 模板日志函数 - 支持格式化输出
    /// </summary>
    template<typename... Args>
    void LogMessage(spdlog::source_loc loc, spdlog::level::level_enum lvl, 
                    spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        spdlog::memory_buf_t buf;
        fmt::vformat_to(fmt::appender(buf), fmt, fmt::make_format_args(args...));
        LogInternal(loc, lvl, buf);
    }

private:
    Log() = default;
    
    static void LogInternal(const spdlog::source_loc& loc, spdlog::level::level_enum lvl,
                    const spdlog::memory_buf_t& buffer);
};

// 内部宏 - 简化日志调用
#define TE_LOG_CALL(level, ...) \
    TE::Log::GetInstance().LogMessage(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, level, __VA_ARGS__)

// 公共日志宏
#define TE_LOG_TRACE(...)    TE_LOG_CALL(spdlog::level::trace, __VA_ARGS__)
#define TE_LOG_DEBUG(...)    TE_LOG_CALL(spdlog::level::debug, __VA_ARGS__)
#define TE_LOG_INFO(...)     TE_LOG_CALL(spdlog::level::info, __VA_ARGS__)
#define TE_LOG_WARN(...)     TE_LOG_CALL(spdlog::level::warn, __VA_ARGS__)
#define TE_LOG_ERROR(...)    TE_LOG_CALL(spdlog::level::err, __VA_ARGS__)
#define TE_LOG_CRITICAL(...) TE_LOG_CALL(spdlog::level::critical, __VA_ARGS__)

} // namespace TE


