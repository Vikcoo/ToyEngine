#include "TELog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"
namespace TE {

	std::shared_ptr<spdlog::logger> TELog::sLoggerInstance = nullptr;
	
	void TELog::Init() {
		// 创建一个异步日志记录器
		sLoggerInstance = spdlog::stdout_color_mt<spdlog::async_factory>("AsyncLogger");
		//设置日志级别为信息
		sLoggerInstance->set_level(spdlog::level::trace);
		// 设置日志格式
		sLoggerInstance->set_pattern("[%Y-%m-%d %H:%M:%S] [%1!L] [%l] [%20s:%-4#] [thread %t] %v");
	}
	
}
