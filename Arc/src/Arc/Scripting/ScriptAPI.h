#pragma once

#include "Arc/Scripting/ScriptEngine.h"

extern "C"
{
	typedef struct _MonoString MonoString;
	typedef struct _MonoArray MonoArray;
}

namespace ArcEngine::Scripting
{
	void Log_Critical(MonoString* msg);
	void Log_Error(MonoString* msg);
	void Log_Warn(MonoString* msg);
	void Log_Info(MonoString* msg);
	void Log_Trace(MonoString* msg);
}
