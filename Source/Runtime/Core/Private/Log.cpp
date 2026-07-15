// ToyEngine Core Module
// 日志系统实现

#include "../Public/Log/Log.h"

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/async.h"
#include "spdlog/common.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef TE_ENGINE_ROOT_DIR
#error "TE_ENGINE_ROOT_DIR must be defined by the Core build target"
#endif

namespace TE {

namespace {

constexpr std::size_t LogQueueSize = 8192;
constexpr std::size_t LogFileMaxSize = 5 * 1024 * 1024;
constexpr std::size_t LogFileMaxCount = 3;

std::shared_ptr<spdlog::logger> s_Logger;

[[nodiscard]] spdlog::level::level_enum ToSpdlogLevel(const LogLevel level) noexcept
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

} // namespace

void Log::Init()
{
    if (s_Logger)
    {
        return;
    }

    const std::filesystem::path logDirectory =
        std::filesystem::path(TE_ENGINE_ROOT_DIR) / "Saved" / "Logs";
    std::filesystem::create_directories(logDirectory);
    const std::filesystem::path logFilePath = logDirectory / "ToyEngine.log";
    const std::string logFileName = logFilePath.generic_string();

    const auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::trace);

    const auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        logFileName, LogFileMaxSize, LogFileMaxCount, false);
    fileSink->set_level(spdlog::level::trace);

    spdlog::init_thread_pool(LogQueueSize, 1);

    const std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};
    s_Logger = std::make_shared<spdlog::async_logger>(
        "ToyEngine",
        sinks.begin(),
        sinks.end(),
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);

    s_Logger->set_level(spdlog::level::trace);
    s_Logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%20s:%-4#] %v");
    s_Logger->flush_on(spdlog::level::err);
    spdlog::set_default_logger(s_Logger);
    spdlog::flush_every(std::chrono::seconds(3));

    TE_LOG_INFO("=== ToyEngine Log System Initialized ===");
    TE_LOG_INFO("Log file: {}", logFileName);
}

void Log::Shutdown() noexcept
{
    if (!s_Logger)
    {
        return;
    }

    s_Logger->flush();
    spdlog::shutdown();
    s_Logger.reset();
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
