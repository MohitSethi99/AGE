#include "arcpch.h"
#include "ScriptEngineRegistry.h"

#include "Arc/Scripting/ScriptAPI.h"

namespace ArcEngine
{
	void ScriptEngineRegistry::RegisterAll()
	{
		mono_add_internal_call("ArcEngine::Log:Fatal_Native", Scripting::Log_Critical);
		mono_add_internal_call("ArcEngine::Log:Error_Native", Scripting::Log_Error);
		mono_add_internal_call("ArcEngine::Log:Warn_Native", Scripting::Log_Warn);
		mono_add_internal_call("ArcEngine::Log:Info_Native", Scripting::Log_Info);
		mono_add_internal_call("ArcEngine::Log:Trace_Native", Scripting::Log_Trace);

		ARC_CORE_INFO("Registered all the internal functions");
	}
}
