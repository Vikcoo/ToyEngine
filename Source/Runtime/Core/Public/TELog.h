#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/spdlog.h"




namespace TE {
	class TELog {
	public:
		// 禁用默认构造函数、拷贝构造函数和赋值运算符
		TELog() = delete;
		TELog(const TELog&) = delete;
		TELog& operator=(const TELog&) = delete;

		static void Init();
		static spdlog::logger* GetLoggerInstance() {
			assert(sLoggerInstance && "sLoggerInstance is null ");
			return sLoggerInstance.get();
		};
	private:
		static std::shared_ptr<spdlog::logger> sLoggerInstance;
	};

// 宏简化日志打印代码
#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(TE::TELog::GetLoggerInstance(), __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(TE::TELog::GetLoggerInstance(), __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(TE::TELog::GetLoggerInstance(), __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(TE::TELog::GetLoggerInstance(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(TE::TELog::GetLoggerInstance(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(TE::TELog::GetLoggerInstance(), __VA_ARGS__)
}
