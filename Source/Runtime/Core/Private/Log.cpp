// ToyEngine Core Module
// 日志系统实现

#include "../Public/Log/Log.h"

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/spdlog.h"
#include "spdlog/common.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"
#include <memory>

namespace TE {

// 全局spdlog日志器
static std::shared_ptr<spdlog::logger> s_Logger = nullptr;

spdlog::level::level_enum ToSpdlogLevel(const LogLevel level)
{
    switch (level)
    {
    case LogLevel::Trace:
        return spdlog::level::trace;
    case LogLevel::Debug:
        return spdlog::level::debug;
    case LogLevel::Info:
        return spdlog::level::info;
    case LogLevel::Warn:
        return spdlog::level::warn;
    case LogLevel::Error:
        return spdlog::level::err;
    case LogLevel::Critical:
        return spdlog::level::critical;
    default:
        return spdlog::level::info;
    }
}

void Log::Init()
{
    // 创建控制台输出sink（带颜色）
    const auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::trace);
    
    // 创建文件输出sink（可选，注释掉如果不需要）
    // auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Saved/Logs/ToyEngine.log", true);
    // fileSink->set_level(spdlog::level::trace);
    
    // 创建异步日志器
    spdlog::init_thread_pool(8192, 1);  // 队列大小8192，1个后台线程
    
    // 组合多个sink
    std::vector<spdlog::sink_ptr> sinks { consoleSink };
    // sinks.push_back(fileSink);  // 如果需要文件输出
    
    s_Logger = std::make_shared<spdlog::async_logger>(
        "ToyEngine", 
        sinks.begin(), 
        sinks.end(),
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block
    );
    
    // 设置日志级别
    s_Logger->set_level(spdlog::level::trace);
    
    // 设置日志格式
    // [时间] [级别] [文件:行号] [线程] 消息
    s_Logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%20s:%-4#] %v");
    
    // 注册为默认日志器
    spdlog::set_default_logger(s_Logger);
    
    TE_LOG_INFO("=== ToyEngine Log System Initialized ===");
}

Log& Log::GetInstance()
{
    static Log instance;
    return instance;
}

void Log::LogInternal(const LogSourceLocation loc, const LogLevel lvl, const std::string_view message)
{
    if (s_Logger)
    {
        const spdlog::source_loc sourceLoc{loc.File, loc.Line, loc.Function};
        s_Logger->log(sourceLoc, ToSpdlogLevel(lvl), message);
    }
}

} // namespace TE
