#pragma once

#include "Arc/Core/Base.h"

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace ArcEngine
{
	enum LogLevel : uint32_t
	{
		Trace = 1,
		Debug = 2,
		Info = 4,
		Warn = 8,
		Error = 16,
		Critical = 32,
	};

	class Log
	{
	public:
		static void Init();
		
		static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static Ref<spdlog::logger>& GetClientLogger() { return  s_ClientLogger; }

	private:
		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
	};
}

#define ARC_CORE_TRACE(...)		::ArcEngine::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define ARC_CORE_INFO(...)		::ArcEngine::Log::GetCoreLogger()->info(__VA_ARGS__)
#define ARC_CORE_WARN(...)		::ArcEngine::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define ARC_CORE_ERROR(...)		::ArcEngine::Log::GetCoreLogger()->error(__VA_ARGS__)
#define ARC_CORE_CRITICAL(...)	::ArcEngine::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define ARC_TRACE(...)			::ArcEngine::Log::GetClientLogger()->trace(__VA_ARGS__)
#define ARC_INFO(...)			::ArcEngine::Log::GetClientLogger()->info(__VA_ARGS__)
#define ARC_WARN(...)			::ArcEngine::Log::GetClientLogger()->warn(__VA_ARGS__)
#define ARC_ERROR(...)			::ArcEngine::Log::GetClientLogger()->error(__VA_ARGS__)
#define ARC_CRITICAL(...)		::ArcEngine::Log::GetClientLogger()->critical(__VA_ARGS__)
