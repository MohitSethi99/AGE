#include "arcpch.h"
#include "ScriptAPI.h"

namespace ArcEngine::Scripting
{
	std::string FromMonoString(MonoString* str)
	{
		mono_unichar2* chl = mono_string_chars(str);
		std::string out("");
		for (int i = 0; i < mono_string_length(str); i++)
			out += chl[i];

		return out;
	}

	void Log_Critical(MonoString* msg) { ARC_CRITICAL(FromMonoString(msg)); }
	void Log_Error(MonoString* msg) { ARC_ERROR(FromMonoString(msg)); }
	void Log_Warn(MonoString* msg) { ARC_WARN(FromMonoString(msg)); }
	void Log_Info(MonoString* msg) { ARC_INFO(FromMonoString(msg)); }
	void Log_Trace(MonoString* msg) { ARC_TRACE(FromMonoString(msg)); }
}
