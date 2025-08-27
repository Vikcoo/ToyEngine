#include "TELog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"


namespace TE {

	TELog TELog::sLoggerInstance{};
	std::shared_ptr<spdlog::logger> sSpdLoger{};

	void TELog::Init() {

		sSpdLoger = spdlog::stdout_color_mt<spdlog::async_factory>("AsyncLogger");

		sSpdLoger->set_level(spdlog::level::trace);

		sSpdLoger->set_pattern("[%Y-%m-%d %H:%M:%S] [%1!L] [%l] [%20s:%-4#] [thread %t] %v");
	}

	void TELog::Log(spdlog::source_loc loc, spdlog::level::level_enum lvl, const spdlog::memory_buf_t &buffer) {
		sSpdLoger->log(loc, lvl, spdlog::string_view_t(buffer.data(), buffer.size()));
	}
}
