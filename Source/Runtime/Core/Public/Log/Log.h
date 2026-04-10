// ToyEngine Core Module
// 日志系统 - 对外仅暴露引擎自有接口

#pragma once

#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace TE {

enum class LogLevel
{
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Critical
};

struct LogSourceLocation
{
    const char* File;
    int Line;
    const char* Function;
};

/// <summary>
/// 日志管理器 - 对外提供稳定日志接口，具体后端隐藏在私有实现中
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
    void LogMessage(LogSourceLocation loc, LogLevel lvl,
                    std::format_string<Args...> fmt, Args&&... args)
    {
        LogInternal(loc, lvl, std::format(fmt, std::forward<Args>(args)...));
    }

private:
    Log() = default;
    
    static void LogInternal(LogSourceLocation loc, LogLevel lvl, std::string_view message);
};

// 内部宏 - 简化日志调用
#define TE_LOG_CALL(level, ...) \
    TE::Log::GetInstance().LogMessage(TE::LogSourceLocation{__FILE__, __LINE__, __func__}, level, __VA_ARGS__)

// 公共日志宏
#define TE_LOG_TRACE(...)    TE_LOG_CALL(TE::LogLevel::Trace, __VA_ARGS__)
#define TE_LOG_DEBUG(...)    TE_LOG_CALL(TE::LogLevel::Debug, __VA_ARGS__)
#define TE_LOG_INFO(...)     TE_LOG_CALL(TE::LogLevel::Info, __VA_ARGS__)
#define TE_LOG_WARN(...)     TE_LOG_CALL(TE::LogLevel::Warn, __VA_ARGS__)
#define TE_LOG_ERROR(...)    TE_LOG_CALL(TE::LogLevel::Error, __VA_ARGS__)
#define TE_LOG_CRITICAL(...) TE_LOG_CALL(TE::LogLevel::Critical, __VA_ARGS__)

} // namespace TE
