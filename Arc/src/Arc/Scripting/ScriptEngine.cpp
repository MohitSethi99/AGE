#include "arcpch.h"
#include "ScriptEngine.h"

#include "ScriptEngineRegistry.h"

namespace ArcEngine
{
	static bool s_Initialized = false;

	static MonoDomain* s_MonoDomain = nullptr;
	static std::string s_AssemblyPath;

	static ScriptModuleFieldMap s_PublicFields;

	static std::unordered_map<std::string, EntityBehaviourClass> s_BehaviourClassMap;
	static std::unordered_map<uint32_t, EntityInstance> s_EntityInstanceMap;

	MonoAssembly* LoadAssemblyFromFile(const char* filepath)
	{
		if (filepath == nullptr)
			return nullptr;

		HANDLE file = CreateFileA(filepath, FILE_READ_ACCESS, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
		{
			return nullptr;
		}

		DWORD fileSize = GetFileSize(file, nullptr);
		if (fileSize == INVALID_FILE_SIZE)
		{
			CloseHandle(file);
			return nullptr;
		}

		void* fileData = malloc(fileSize);
		if (fileData == nullptr)
		{
			CloseHandle(file);
			return nullptr;
		}

		DWORD read = 0;
		ReadFile(file, fileData, fileSize, &read, nullptr);
		if (fileSize != read)
		{
			free(fileData);
			CloseHandle(file);
			return nullptr;
		}

		MonoImageOpenStatus status;
		MonoImage* image = mono_image_open_from_data_full(reinterpret_cast<char*>(fileData), fileSize, true, &status, false);
		if (status != MONO_IMAGE_OK)
		{
			return nullptr;
		}

		auto assemb = mono_assembly_load_from_full(image, filepath, &status, false);
		free(fileData);
		CloseHandle(file);
		mono_image_close(image);
		return assemb;
	}

	static MonoAssembly* LoadAssembly(const std::string& path)
	{
		MonoAssembly* assembly = LoadAssemblyFromFile(path.c_str());

		if (!assembly)
			ARC_CORE_ERROR("Could not load assembly: {0}", path);
		else
			ARC_CORE_INFO("Successfully loaded assembly: {0}", path);

		return assembly;
	}

	static MonoImage* GetAssemblyImage(MonoAssembly* assembly)
	{
		MonoImage* image = mono_assembly_get_image(assembly);
		if (!image)
			ARC_CORE_ERROR("mono_assembly_get_image failed!");

		return image;
	}

	static MonoClass* GetClass(MonoImage* image, const EntityBehaviourClass& behaviourClass)
	{
		MonoClass* monoClass = mono_class_from_name(image, behaviourClass.NamespaceName.c_str(), behaviourClass.ClassName.c_str());
		return monoClass;
	}

	static MonoObject* Instantiate(EntityBehaviourClass& behaviourClass)
	{
		MonoObject* instance = mono_object_new(s_MonoDomain, behaviourClass.Class);
		if (!instance)
			ARC_CORE_ERROR("mono_object_new failed!");

		return instance;
		/*
		mono_runtime_object_init(instance);
		uint32_t handle = mono_gchandle_new(instance, false);
		return handle;
		*/
	}

	
	static MonoAssembly* s_AppAssembly = nullptr;
	static MonoAssembly* s_CoreAssembly = nullptr;
	MonoImage* s_AppAssemblyImage = nullptr;
	MonoImage* s_CoreAssemblyImage = nullptr;

	bool ScriptEngine::HasClass(BehaviourComponent& behaviour)
	{
		if (!s_Initialized)
			return false;

		std::string namespaceName;
		std::string className;
		if (behaviour.ModuleName.find('.') != std::string::npos)
		{
			size_t dotPos = behaviour.ModuleName.find_last_of('.');
			namespaceName = behaviour.ModuleName.substr(0, dotPos);
			className = behaviour.ModuleName.substr(dotPos + 1);
		}
		else
		{
			className = behaviour.ModuleName;
		}

		MonoClass* monoClass = mono_class_from_name(s_AppAssemblyImage, namespaceName.c_str(), className.c_str());
		if (monoClass == nullptr)
			return false;
		
		return true;
	}
	
	void ScriptEngine::Init(const std::string& assemblyPath)
	{
		s_Initialized = true;
		s_AssemblyPath = assemblyPath;
		
		mono_set_dirs("C:\\Program Files\\Mono\\lib",
		"C:\\Program Files\\Mono\\etc");
		
		mono_jit_set_trace_options("--verbose");

		if (s_AppAssembly)
		{
			mono_domain_unload(s_MonoDomain);
			mono_assembly_close(s_AppAssembly);
		}

		//Init a domain
		s_MonoDomain = mono_jit_init("ArcRuntime");

		//Open a assembly in the domain
		//s_CoreAssembly = mono_domain_assembly_open(s_MonoDomain, "assets/scripts/ArcSharp.dll");
		//s_CoreAssemblyImage = mono_assembly_get_image(s_CoreAssembly);

		s_AppAssembly = mono_domain_assembly_open(s_MonoDomain, assemblyPath.c_str());
		s_AppAssemblyImage = mono_assembly_get_image(s_AppAssembly);
		
		ScriptEngineRegistry::RegisterAll();
	}

	void ScriptEngine::Shutdown()
	{
	}

	void ScriptEngine::OnCreate(Entity entity)
	{
		EntityInstance& entityInstance = s_EntityInstanceMap[(uint32_t) entity];
		if (entityInstance.ScriptClass->MethodOnCreate)
			MonoUtils::CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->MethodOnCreate);
	}

	void ScriptEngine::OnUpdate(uint32_t entityID, Timestep ts)
	{
		if (!s_Initialized)
			return;

		ARC_CORE_ASSERT(s_EntityInstanceMap.find(entityID) != s_EntityInstanceMap.end(), "Could not find entity in instance map!");

		auto& entity = s_EntityInstanceMap[entityID];

		if (entity.ScriptClass->MethodOnUpdate)
		{
			void* args[] = { &ts };
			MonoUtils::CallMethod(entity.GetInstance(), entity.ScriptClass->MethodOnUpdate, args);
		}
	}

	void ScriptEngine::OnInit(BehaviourComponent& behaviour, uint32_t entityID, uint32_t sceneID)
	{
		if (!s_Initialized)
			return;

		EntityBehaviourClass& behaviourClass = s_BehaviourClassMap[behaviour.ModuleName];
		behaviourClass.FullName = behaviour.ModuleName;
		if (behaviour.ModuleName.find('.') != std::string::npos)
		{
			size_t dotPos = behaviour.ModuleName.find_last_of('.');
			behaviourClass.NamespaceName = behaviour.ModuleName.substr(0, dotPos);
			behaviourClass.ClassName = behaviour.ModuleName.substr(dotPos + 1);
		}
		else
		{
			behaviourClass.ClassName = behaviour.ModuleName;
		}

		behaviourClass.Class = GetClass(s_AppAssemblyImage, behaviourClass);
		if (behaviourClass.Class == nullptr)
			return;

		behaviourClass.InitClassMethods(s_AppAssemblyImage);

		EntityInstance& entityInstance = s_EntityInstanceMap[entityID];
		entityInstance.ScriptClass = &behaviourClass;
		entityInstance.Instance = Instantiate(behaviourClass);

		MonoProperty* entityIDProperty = mono_class_get_property_from_name(behaviourClass.Class, "EntityID");
		mono_property_get_get_method(entityIDProperty);
		MonoMethod* entityIDSetMethod = mono_property_get_set_method(entityIDProperty);
		void* param[] = { &entityID };
		MonoUtils::CallMethod(entityInstance.GetInstance(), entityIDSetMethod, param);

		MonoProperty* sceneIDProperty = mono_class_get_property_from_name(behaviourClass.Class, "SceneID");
		mono_property_get_get_method(sceneIDProperty);
		MonoMethod* sceneIDSetMethod = mono_property_get_set_method(sceneIDProperty);
		param[0] = { &sceneID };
		MonoUtils::CallMethod(entityInstance.GetInstance(), sceneIDSetMethod, param);

		if (behaviourClass.MethodOnCreate)
			MonoUtils::CallMethod(entityInstance.GetInstance(), behaviourClass.MethodOnCreate, nullptr);

		// Retrieve public fields
		{
			MonoClassField* iter;
			void* ptr = 0;
			while ((iter = mono_class_get_fields(behaviourClass.Class, &ptr)) != NULL)
			{
				const char* name = mono_field_get_name(iter);
				uint32_t flags = mono_field_get_flags(iter);
				if (flags & MONO_FIELD_ATTR_PUBLIC == 0)
					continue;

				MonoType* fieldType = mono_field_get_type(iter);
				FieldType xalocFieldType = GetFieldType(fieldType);

				// TODO: Attributes
				MonoCustomAttrInfo* attr = mono_custom_attrs_from_field(behaviourClass.Class, iter);

				auto& publicField = s_PublicFields[behaviour.ModuleName].emplace_back(name, xalocFieldType);
				publicField.m_EntityInstance = &entityInstance;
				publicField.m_MonoClassField = iter;
				// mono_field_set_value(entityInstance.Instance, iter, )
			}
		}
	}

	const ScriptModuleFieldMap& ScriptEngine::GetFieldMap()
	{
		return s_PublicFields;
	}
}
