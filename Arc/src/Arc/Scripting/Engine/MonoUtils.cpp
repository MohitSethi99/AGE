#include "arcpch.h"
#include "MonoUtils.h"

namespace ArcEngine
{
	MonoMethod* ArcEngine::MonoUtils::GetMethod(MonoImage* image, const std::string& methodDesc)
	{
		MonoMethodDesc* desc = mono_method_desc_new(methodDesc.c_str(), false);
		if (!desc)
			ARC_CORE_ERROR("mono_method_desc_new failed!");

		MonoMethod* method = mono_method_desc_search_in_image(desc, image);
		if (!method)
			ARC_CORE_ERROR("mono_method_desc_search_in_image failed!");

		return method;
	}

	MonoObject* ArcEngine::MonoUtils::CallMethod(MonoObject* object, MonoMethod* method, void** params)
	{
		MonoObject* pException = nullptr;
		MonoObject* result = mono_runtime_invoke(method, object, params, &pException);
		return result;
	}
}
