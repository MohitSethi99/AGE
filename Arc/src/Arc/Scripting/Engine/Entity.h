#pragma once

#include "MonoUtils.h"
#include "Arc/Scene/Scene.h"

extern "C"
{
	typedef struct _MonoObject MonoObject;
}

namespace ArcEngine
{
	struct EntityBehaviourClass
	{
		std::string FullName;
		std::string ClassName;
		std::string NamespaceName;

		MonoClass* Class;
		MonoMethod* MethodOnCreate;
		MonoMethod* MethodOnDestroy;
		MonoMethod* MethodOnUpdate;

		void InitClassMethods(MonoImage* image)
		{
			MethodOnCreate = MonoUtils::GetMethod(image, FullName + ":OnCreate()");
//			MethodOnCreate = MonoUtils::GetMethod(image, FullName + ":OnUpdate(single)");
		}
	};

	struct EntityInstance
	{
		EntityBehaviourClass* ScriptClass;

		MonoObject* Instance;
		Scene* SceneInstance;

		MonoObject* GetInstance()
		{
			return Instance;
		}
	};
}
