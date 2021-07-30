#pragma once

#include "Entity.h"

#include <mono/metadata/debug-helpers.h>

extern "C"
{
	typedef struct _MonoClassField MonoClassField;
}

namespace ArcEngine
{
	enum class FieldType
	{
		None = 0, Float, Int, UnsignedInt, String, Vec2, Vec3, Vec4
	};

	static FieldType GetFieldType(MonoType* monoType)
	{
		int type = mono_type_get_type(monoType);
		switch (type)
		{
			case MONO_TYPE_R4: return FieldType::Float;
			case MONO_TYPE_I4: return FieldType::Int;
			case MONO_TYPE_U4: return FieldType::UnsignedInt;
			case MONO_TYPE_STRING: return FieldType::String;
			case MONO_TYPE_VALUETYPE:
				{
					char* name = mono_type_get_name(monoType);
					if (strcmp(name, "Vector2") == 0) return FieldType::Vec2;
					if (strcmp(name, "Vector3") == 0) return FieldType::Vec3;
					if (strcmp(name, "Vector4") == 0) return FieldType::Vec4;
				}
		}

		return FieldType::None;
	}

	struct PublicField
	{
		std::string Name;
		FieldType Type;

		PublicField(const std::string& name, FieldType type)
			: Name(name), Type(type) {}

		template<typename T>
		T GetValue() const
		{
			T value;
			GetValue_Internal(&value);
			return value;
		}

		template<typename T>
		void SetValue(T value) const
		{
			SetValue_Internal(&value);
		}

	private:
		EntityInstance* m_EntityInstance;
		MonoClassField* m_MonoClassField;

		void SetValue_Internal(void* value) const;
		void GetValue_Internal(void* outValue) const;

		friend class ScriptEngine;
	};
}
