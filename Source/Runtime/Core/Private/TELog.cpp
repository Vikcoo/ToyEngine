#include "TELog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"
namespace TE {

	std::shared_ptr<spdlog::logger> TELog::sLoggerInstance = nullptr;
	
	void TELog::Init() {
		// ����һ���첽��־��¼��
		sLoggerInstance = spdlog::stdout_color_mt<spdlog::async_factory>("AsyncLogger");
		//������־����Ϊ��Ϣ
		sLoggerInstance->set_level(spdlog::level::trace);
		// ������־��ʽ
		sLoggerInstance->set_pattern("[%Y-%m-%d %H:%M:%S] [%1!L] [%l] [%20s:%-4#] [thread %t] %v");
	}
	
}
