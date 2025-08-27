#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include "spdlog/common.h"

namespace TE {
    class TELog {
    public:
        // 禁用默认构造函数、拷贝构造函数和赋值运算符
        TELog() = default;
        TELog(const TELog&) = delete;
        TELog& operator=(const TELog&) = delete;

        static void Init();
        static TELog* GetLoggerInstance() {
            return &sLoggerInstance;
        };

        template<typename... Args>
        void Log(spdlog::source_loc loc, spdlog::level::level_enum lvl, spdlog::format_string_t<Args...> fmt, Args&&... args) {
            spdlog::memory_buf_t buf;
            fmt::vformat_to(fmt::appender(buf), fmt, fmt::make_format_args(args...));
            Log(loc,lvl,buf);
        }





    private:
        void Log(spdlog::source_loc loc, spdlog::level::level_enum lvl, const spdlog::memory_buf_t& buffer);
        static TELog sLoggerInstance;
    };

#define TE_LOG_LOGGER_CALL(TELog,level, ...)\
TELog->Log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, level, __VA_ARGS__)

    // 宏简化日志打印代码
#define LOG_TRACE(...)    TE_LOG_LOGGER_CALL(TE::TELog::GetLoggerInstance(),spdlog::level::trace, __VA_ARGS__)
#define LOG_DEBUG(...)    TE_LOG_LOGGER_CALL(TE::TELog::GetLoggerInstance(),spdlog::level::debug, __VA_ARGS__)
#define LOG_INFO(...)     TE_LOG_LOGGER_CALL(TE::TELog::GetLoggerInstance(),spdlog::level::info, __VA_ARGS__)
#define LOG_WARN(...)     TE_LOG_LOGGER_CALL(TE::TELog::GetLoggerInstance(),spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(...)    TE_LOG_LOGGER_CALL(TE::TELog::GetLoggerInstance(),spdlog::level::err, __VA_ARGS__)
}
