#pragma once

#include "Arc/Scripting/Engine/Field.h"

#include "Arc/Core/Timestep.h"
#include "Arc/Scene/Components.h"
#include "Arc/Scene/Entity.h"

#include <string>

extern "C"
{
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoClassField MonoClassField;
}

namespace ArcEngine
{
	using ScriptModuleFieldMap = std::unordered_map<std::string, std::vector<PublicField>>;

	class ScriptEngine
	{
	public:
		static void Init(const std::string& assemblyPath);
		static void Shutdown();

		static bool HasClass(BehaviourComponent& behaviour);
		
		static void OnCreate(Entity entity);
		static void OnUpdate(uint32_t entityID, Timestep ts);

		static void OnInit(BehaviourComponent& behaviour, uint32_t entityID, uint32_t sceneID);
		static const ScriptModuleFieldMap& GetFieldMap();
	};
}
