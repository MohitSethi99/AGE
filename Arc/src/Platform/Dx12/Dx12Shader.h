#pragma once

#include "Arc/Core/Base.h"
#include "Arc/Renderer/Shader.h"

struct IDxcBlob;

namespace ArcEngine
{
	class Dx12Shader : public Shader
	{
	public:
		explicit Dx12Shader(const std::filesystem::path& filepath, ShaderType type);
		~Dx12Shader() override;

		Dx12Shader(const Dx12Shader& other) = default;
		Dx12Shader(Dx12Shader&& other) = default;

		void Recompile(const std::filesystem::path& path) override;

		[[nodiscard]] const eastl::string& GetName() const override;

	private:
		void Compile(const std::filesystem::path& filepath);

	private:
		friend class Dx12PipelineState;

		eastl::string								m_Name;
		ShaderType									m_Type;
		eastl::hash_map<ShaderType, IDxcBlob*>		m_ShaderBlobs;
		eastl::hash_map<ShaderType, IDxcBlob*>		m_ReflectionBlobs;
	};
}
