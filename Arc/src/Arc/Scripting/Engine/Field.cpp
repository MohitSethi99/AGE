#include "arcpch.h"
#include "Field.h"

namespace ArcEngine
{
	void PublicField::SetValue_Internal(void* value) const
	{
		mono_field_set_value(m_EntityInstance->GetInstance(), m_MonoClassField, value);
	}

	void PublicField::GetValue_Internal(void* outValue) const
	{
		mono_field_get_value(m_EntityInstance->GetInstance(), m_MonoClassField, outValue);
	}
}
