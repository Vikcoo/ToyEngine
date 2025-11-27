// ToyEngine Core Module
// 日志系统实现

#include "../Public/Log/Log.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"
#include <memory>

namespace TE {

// 全局spdlog日志器
static std::shared_ptr<spdlog::logger> s_Logger = nullptr;

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

void Log::LogInternal(const spdlog::source_loc& loc, const spdlog::level::level_enum lvl,
                     const spdlog::memory_buf_t& buffer)
{
    if (s_Logger)
    {
        s_Logger->log(loc, lvl, spdlog::string_view_t(buffer.data(), buffer.size()));
    }
}

} // namespace TE

