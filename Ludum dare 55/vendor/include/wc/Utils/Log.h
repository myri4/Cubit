#pragma once

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/base_sink.h>
#include <iostream>
#include <mutex>

namespace wc 
{
	template<typename Mutex>
	struct ConsoleSink : public spdlog::sinks::base_sink <Mutex>
	{
		struct Message
		{
			std::string loggerName;
			spdlog::level::level_enum level;
			std::chrono::system_clock::time_point time;
			size_t thread_id{ 0 };
			const char* filename = nullptr;
			const char* funcname = nullptr;
			uint32_t line = 0;
			std::string payload;
		};

		std::vector<Message> messages;
	protected:
		void sink_it_(const spdlog::details::log_msg& msg) override
		{
			spdlog::memory_buf_t formatted;
			spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
			Message fmsg = {};
			fmsg.loggerName = std::string(msg.logger_name.data());
			fmsg.level = msg.level;
			fmsg.time = msg.time;
			fmsg.thread_id = msg.thread_id;
			fmsg.filename = msg.source.filename;
			fmsg.funcname = msg.source.funcname;
			fmsg.line = msg.source.line;
			fmsg.payload = std::string(msg.payload.data());
			messages.push_back(fmsg);
			//std::cout << fmt::to_string(formatted);
		}

		void flush_() override
		{
			//std::cout << std::flush;
		}
	};

	using ConsoleSinkMt = ConsoleSink<std::mutex>;

	struct Log
	{
		static void Init()
		{
			GetConsoleSink() = std::make_shared<ConsoleSinkMt>();

			std::array<spdlog::sink_ptr, 3> logSinks;
			logSinks[0] = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			logSinks[1] = std::make_shared<spdlog::sinks::basic_file_sink_mt>("log/log.txt", true);
			logSinks[2] = GetConsoleSink();

			logSinks[0]->set_pattern("%^[%D %T][%l] %n: %v%$");
			logSinks[1]->set_pattern("%^[%D %T][%l] %n: %v%$");
			logSinks[2]->set_pattern("%^[%D %T][%l] %n: %v%$");

			GetCoreLogger() = std::make_shared<spdlog::logger>("Core", begin(logSinks), std::prev(end(logSinks)));
			GetAppLogger() = std::make_shared<spdlog::logger>("App", std::next(begin(logSinks)), end(logSinks));
			spdlog::register_logger(Log::GetCoreLogger());
			spdlog::register_logger(Log::GetAppLogger());

			GetAppLogger()->set_level(spdlog::level::trace);
			GetCoreLogger()->set_level(spdlog::level::trace);
		}

		static std::shared_ptr<spdlog::logger>& GetAppLogger() 
		{ 
			static std::shared_ptr<spdlog::logger> s_AppLogger;
			return s_AppLogger; 
		}

		static std::shared_ptr<spdlog::logger>& GetCoreLogger()
		{
			static std::shared_ptr<spdlog::logger> s_CoreLogger;
			return s_CoreLogger;
		}

		static std::shared_ptr<ConsoleSinkMt>& GetConsoleSink()
		{
			static std::shared_ptr<ConsoleSinkMt> s_ConsoleSink;
			return s_ConsoleSink;
		}
	};
}
// Core log macros
#define WC_CORE_TRACE(...)         wc::Log::GetCoreLogger()->trace(__VA_ARGS__);
#define WC_CORE_INFO(...)          wc::Log::GetCoreLogger()->info(__VA_ARGS__);
#define WC_CORE_WARN(...)          wc::Log::GetCoreLogger()->warn(__VA_ARGS__);
#define WC_CORE_ERROR(...)         wc::Log::GetCoreLogger()->error(__VA_ARGS__);
#define WC_CORE_CRITICAL(...)      wc::Log::GetCoreLogger()->critical(__VA_ARGS__);
#define WC_CORE_DEBUG(...)         wc::Log::GetCoreLogger()->debug(__VA_ARGS__);
#define WC_CORE_TODO WC_CORE_INFO("@TODO: Implement!")

#define WC_TRACE(...)         wc::Log::GetAppLogger()->trace(__VA_ARGS__);
#define WC_INFO(...)          wc::Log::GetAppLogger()->info(__VA_ARGS__);
#define WC_WARN(...)          wc::Log::GetAppLogger()->warn(__VA_ARGS__);
#define WC_ERROR(...)         wc::Log::GetAppLogger()->error(__VA_ARGS__);
#define WC_CRITICAL(...)      wc::Log::GetAppLogger()->critical(__VA_ARGS__);
#define WC_DEBUG(...)         wc::Log::GetAppLogger()->debug(__VA_ARGS__);

#define WC_ENABLE_ASSERTS


#ifdef WC_ENABLE_ASSERTS
// Currently accepts at least the condition and one additional parameter (the message) being optional
#define WC_DEBUGBREAK() __debugbreak();
#define WC_ASSERT(x) assert(x);
#else
#define WC_DEBUGBREAK(x)
#define WC_ASSERT(...)
#endif