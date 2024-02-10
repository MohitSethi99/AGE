#include "arcpch.h"
#include "Dx12PipelineState.h"

#include <dxcapi.h>
#include <d3d12shader.h>

#include "Arc/Utils/StlHelper.h"
#include "DxHelper.h"
#include "Dx12Framebuffer.h"
#include "Dx12Shader.h"

namespace ArcEngine
{
	static DXGI_FORMAT GetFormatFromMaskComponents(BYTE mask, D3D_REGISTER_COMPONENT_TYPE componentType)
	{
		if (mask == 1)
		{
			if (componentType == D3D_REGISTER_COMPONENT_UINT32)			return DXGI_FORMAT_R32_UINT;
			if (componentType == D3D_REGISTER_COMPONENT_SINT32)			return DXGI_FORMAT_R32_SINT;
			if (componentType == D3D_REGISTER_COMPONENT_FLOAT32)		return DXGI_FORMAT_R32_FLOAT;
		}
		else if (mask <= 3)
		{
			if (componentType == D3D_REGISTER_COMPONENT_UINT32)			return DXGI_FORMAT_R32G32_UINT;
			if (componentType == D3D_REGISTER_COMPONENT_SINT32)			return DXGI_FORMAT_R32G32_SINT;
			if (componentType == D3D_REGISTER_COMPONENT_FLOAT32)		return DXGI_FORMAT_R32G32_FLOAT;
		}
		else if (mask <= 7)
		{
			if (componentType == D3D_REGISTER_COMPONENT_UINT32)			return DXGI_FORMAT_R32G32B32_UINT;
			if (componentType == D3D_REGISTER_COMPONENT_SINT32)			return DXGI_FORMAT_R32G32B32_SINT;
			if (componentType == D3D_REGISTER_COMPONENT_FLOAT32)		return DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if (mask <= 15)
		{
			if (componentType == D3D_REGISTER_COMPONENT_UINT32)			return DXGI_FORMAT_R32G32B32A32_UINT;
			if (componentType == D3D_REGISTER_COMPONENT_SINT32)			return DXGI_FORMAT_R32G32B32A32_SINT;
			if (componentType == D3D_REGISTER_COMPONENT_FLOAT32)		return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		return DXGI_FORMAT_UNKNOWN;
	}

	static MaterialPropertyType GetVariableType(const D3D12_SHADER_TYPE_DESC& desc)
	{
		switch (desc.Type)
		{
			case D3D_SVT_TEXTURE2D:			return MaterialPropertyType::Texture2D;
			case D3D_SVT_BOOL:				return MaterialPropertyType::Bool;
			case D3D_SVT_INT:				return MaterialPropertyType::Int;
			case D3D_SVT_UINT:				return MaterialPropertyType::UInt;
			case D3D_SVT_FLOAT:
			{
				if (desc.Columns == 1)		return MaterialPropertyType::Float;
				if (desc.Columns == 2)		return MaterialPropertyType::Float2;
				if (desc.Columns == 3)		return MaterialPropertyType::Float3;
				if (desc.Columns == 4)		return MaterialPropertyType::Float4;
			}

			default:						return MaterialPropertyType::None;
		}
	}

	static void AppendMaterials(const Microsoft::WRL::ComPtr<ID3D12ShaderReflection>& reflection,
		eastl::vector<D3D12_ROOT_PARAMETER1>& outRootParams,
		eastl::array<D3D12_DESCRIPTOR_RANGE1, 20>& outDescriptors,
		uint32_t* outDescriptorsEnd,
		eastl::vector<MaterialProperty>& outMaterialProperties,
		eastl::hash_map<eastl::string, uint32_t>& bufferMap)
	{
		ARC_PROFILE_SCOPE();

		D3D12_SHADER_DESC shaderDesc;
		reflection->GetDesc(&shaderDesc);

		outRootParams.reserve(outRootParams.size() + shaderDesc.BoundResources);
		for (uint32_t i = 0; i < shaderDesc.BoundResources; ++i)
		{
			D3D12_SHADER_INPUT_BIND_DESC shaderInputBindDesc{};
			reflection->GetResourceBindingDesc(i, &shaderInputBindDesc);

			ID3D12ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByIndex(i);
			D3D12_SHADER_BUFFER_DESC constantBufferDesc{};
			cb->GetDesc(&constantBufferDesc);

			eastl::string bufferName { shaderInputBindDesc.Name };

			if (bufferMap.find(bufferName) != bufferMap.end())
				continue;

			bool tex = shaderInputBindDesc.Type == D3D_SIT_TEXTURE;
			bool cbuffer = shaderInputBindDesc.Type == D3D_SIT_CBUFFER;
			bool bindlessTextures = cbuffer && bufferName == BindlessTexturesSlotName;
			bool materialProperties = bufferName == MaterialPropertiesSlotName;

			CD3DX12_ROOT_PARAMETER1 rootParameter;
			switch (shaderInputBindDesc.Type)
			{
				case D3D_SIT_CBUFFER:
					{
						if (bufferName == "Transform")
						{
							rootParameter.InitAsConstants(16u, shaderInputBindDesc.BindPoint, shaderInputBindDesc.Space);
						}
						else if (bufferName == "Textures")
						{
							rootParameter.InitAsConstants(32u, shaderInputBindDesc.BindPoint, shaderInputBindDesc.Space);
						}
						else
						{
							rootParameter.InitAsConstantBufferView(shaderInputBindDesc.BindPoint, shaderInputBindDesc.Space);
							rootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
						}
					}
					break;
				case D3D_SIT_TEXTURE:
					{
						CD3DX12_DESCRIPTOR_RANGE1 range;
						range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shaderInputBindDesc.BindCount, shaderInputBindDesc.BindPoint, shaderInputBindDesc.Space);
						size_t index = *outDescriptorsEnd;
						outDescriptors[index] = range;
						rootParameter.InitAsDescriptorTable(1, &outDescriptors[index]);
						++(*outDescriptorsEnd);
					}
					break;
				case D3D_SIT_STRUCTURED:
					{
						CD3DX12_DESCRIPTOR_RANGE1 range;
						range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shaderInputBindDesc.BindCount, shaderInputBindDesc.BindPoint, shaderInputBindDesc.Space, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
						size_t index = *outDescriptorsEnd;
						outDescriptors[index] = range;
						rootParameter.InitAsDescriptorTable(1, &outDescriptors[index]);
						++(*outDescriptorsEnd);
					}
					break;

				case D3D_SIT_UAV_RWTYPED:
					{
						CD3DX12_DESCRIPTOR_RANGE1 range;
						range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, shaderInputBindDesc.BindCount, shaderInputBindDesc.BindPoint, shaderInputBindDesc.Space);
						size_t index = *outDescriptorsEnd;
						outDescriptors[index] = range;
						rootParameter.InitAsDescriptorTable(1, &outDescriptors[index]);
						++(*outDescriptorsEnd);
					}
					break;
				case D3D_SIT_SAMPLER:
				case D3D_SIT_TBUFFER:
				case D3D_SIT_UAV_RWSTRUCTURED:
				case D3D_SIT_BYTEADDRESS:
				case D3D_SIT_UAV_RWBYTEADDRESS:
				case D3D_SIT_UAV_APPEND_STRUCTURED:
				case D3D_SIT_UAV_CONSUME_STRUCTURED:
				case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
				case D3D_SIT_RTACCELERATIONSTRUCTURE:
				case D3D_SIT_UAV_FEEDBACKTEXTURE:
				default: continue;
			}
			const int32_t slot = static_cast<int32_t>(outRootParams.size());
			outRootParams.emplace_back(rootParameter);

			if (bindlessTextures || materialProperties)
			{
				for (uint32_t var = 0; var < constantBufferDesc.Variables; ++var)
				{
					ID3D12ShaderReflectionVariable* variable = cb->GetVariableByIndex(var);
					ID3D12ShaderReflectionType* variableType = variable->GetType();
					D3D12_SHADER_TYPE_DESC variableTypeDesc;
					variableType->GetDesc(&variableTypeDesc);
					MaterialPropertyType type = GetVariableType(variableTypeDesc);
					if (type == MaterialPropertyType::None)
					{
						ARC_CORE_ERROR("Unsupported type in shader cbuffer!");
						continue;
					}

					D3D12_SHADER_VARIABLE_DESC variableDesc;
					variable->GetDesc(&variableDesc);

					std::string variableName = variableDesc.Name;

					if (bindlessTextures && type == MaterialPropertyType::UInt)
						type = MaterialPropertyType::Texture2DBindless;

					MaterialProperty property{};
					property.Name = variableDesc.Name;
					property.DisplayName = variableDesc.Name + (property.IsSlider ? 2 : 0);
					property.SizeInBytes = variableDesc.Size;
					property.StartOffsetInBytes = variableDesc.StartOffset;
					property.Slot = slot;
					property.IsSlider = variableName.ends_with("01");
					property.IsColor = variableName.find("color") != eastl::string::npos || variableName.find("Color") != eastl::string::npos;
					property.Type = type;

					outMaterialProperties.push_back(property);
					bufferMap.emplace(variableName.c_str(), slot);
				}
			}
			else if(tex)
			{
				MaterialProperty property{};
				property.Name = bufferName;
				property.DisplayName = bufferName;
				property.SizeInBytes = sizeof(uint32_t);
				property.StartOffsetInBytes = 0;
				property.Slot = slot;
				property.IsSlider = false;
				property.IsColor = false;
				property.Type = MaterialPropertyType::Texture2D;

				outMaterialProperties.push_back(property);
			}

			bufferMap.emplace(bufferName, slot);
		}
	}

	Dx12PipelineState::Dx12PipelineState(const Ref<Shader>& shader, const PipelineSpecification& spec)
		: m_Specification(spec)
	{
		ARC_PROFILE_SCOPE();

		switch(m_Specification.Type)
		{
			case ShaderType::None:  ARC_CORE_ASSERT(false, "Failed to create pipeline with unknown type"); break;
			case ShaderType::Vertex:
			case ShaderType::Pixel: MakeGraphicsPipeline(shader); break;
			case ShaderType::Compute: MakeComputePipeline(shader); break;
			default: ARC_CORE_ASSERT(false, "Failed to create pipeline with unknown type"); break;
		}
	}

	Dx12PipelineState::~Dx12PipelineState()
	{
		ARC_PROFILE_SCOPE();

		for (auto& [slot, cb] : m_ConstantBufferMap)
		{
			for (uint32_t i = 0; i < Dx12Context::FrameCount; ++i)
			{
				Dx12Context::GetSrvHeap()->Free(cb.Handle[i]);
				if (cb.Allocation[i])
					Dx12Context::DeferredRelease(cb.Allocation[i]);
			}
		}

		for (auto& [slot, sb] : m_StructuredBufferMap)
		{
			for (uint32_t i = 0; i < Dx12Context::FrameCount; ++i)
			{
				Dx12Context::GetSrvHeap()->Free(sb.Handle[i]);
				if (sb.Allocation[i])
					Dx12Context::DeferredRelease(sb.Allocation[i]);
			}
		}

		if (m_PipelineState)
			Dx12Context::DeferredRelease(m_PipelineState);
		if (m_RootSignature)
			Dx12Context::DeferredRelease(m_RootSignature);
	}

	void Dx12PipelineState::Recompile(const Ref<Shader>& shader)
	{

		for (auto& [slot, cb] : m_ConstantBufferMap)
		{
			for (uint32_t i = 0; i < Dx12Context::FrameCount; ++i)
			{
				Dx12Context::GetSrvHeap()->Free(cb.Handle[i]);
				if (cb.Allocation[i])
					Dx12Context::DeferredRelease(cb.Allocation[i]);
			}
		}

		for (auto& [slot, sb] : m_StructuredBufferMap)
		{
			for (uint32_t i = 0; i < Dx12Context::FrameCount; ++i)
			{
				Dx12Context::GetSrvHeap()->Free(sb.Handle[i]);
				if (sb.Allocation[i])
					Dx12Context::DeferredRelease(sb.Allocation[i]);
			}
		}

		if (m_PipelineState)
			Dx12Context::DeferredRelease(m_PipelineState);
		if (m_RootSignature)
			Dx12Context::DeferredRelease(m_RootSignature);

		m_MaterialProperties.clear();
		m_BufferMap.clear();
		m_CrcBufferMap.clear();
		m_ConstantBufferMap.clear();
		m_StructuredBufferMap.clear();

		switch (m_Specification.Type)
		{
			case ShaderType::None:  ARC_CORE_ASSERT(false, "Failed to create pipeline with unknown type"); break;
			case ShaderType::Vertex:
			case ShaderType::Pixel: MakeGraphicsPipeline(shader); break;
			case ShaderType::Compute: MakeComputePipeline(shader); break;
			default: ARC_CORE_ASSERT(false, "Failed to create pipeline with unknown type"); break;
		}
	}

	bool Dx12PipelineState::Bind(GraphicsCommandList commandList) const
	{
		ARC_PROFILE_SCOPE();

		if (!m_PipelineState || !m_RootSignature)
			return false;

		auto* cmdList = reinterpret_cast<D3D12GraphicsCommandList*>(commandList);

		if (m_Specification.Type == ShaderType::Vertex || m_Specification.Type == ShaderType::Pixel)
		{
			switch (m_Specification.GraphicsPipelineSpecs.Primitive)
			{
				case PrimitiveType::Triangle:	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); break;
				case PrimitiveType::Line:		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST); break;
				case PrimitiveType::Point:		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST); break;
				default: ARC_CORE_ASSERT(false, "Unsupported Primitive Type");
			}

			cmdList->SetGraphicsRootSignature(m_RootSignature);
		}
		else if (m_Specification.Type == ShaderType::Compute)
		{
			cmdList->SetComputeRootSignature(m_RootSignature);
		}

		cmdList->SetPipelineState(m_PipelineState);
		return true;
	}

	void Dx12PipelineState::Unbind(GraphicsCommandList commandList) const
	{
		ARC_PROFILE_SCOPE();

		auto* cmdList = reinterpret_cast<D3D12GraphicsCommandList*>(commandList);
		cmdList->SetGraphicsRootSignature(nullptr);
		cmdList->SetPipelineState(nullptr);
	}

	void Dx12PipelineState::SetRSData(GraphicsCommandList commandList, uint32_t crc, const void* data, uint32_t size, uint32_t offset)
	{
		ARC_PROFILE_SCOPE();

		eastl::hash_map<uint32_t, uint32_t>::iterator it = m_CrcBufferMap.find(crc);
		if (it == m_CrcBufferMap.end())
		{
			ARC_CORE_ERROR("Failed to register root signature data {}, Not found!", crc);
			return;
		}

		auto* cmdList = reinterpret_cast<D3D12GraphicsCommandList*>(commandList);
		cmdList->SetGraphicsRoot32BitConstants(it->second, size / 4, data, offset);
	}

	void Dx12PipelineState::MakeGraphicsPipeline(const Ref<Shader>& shader)
	{
		using Microsoft::WRL::ComPtr;

		const GraphicsPipelineSpecification& graphicsSpec = m_Specification.GraphicsPipelineSpecs;

		const Dx12Shader* dxShader = reinterpret_cast<const Dx12Shader*>(shader.get());
		const ComPtr<IDxcBlob> vertexShader = eastl::try_at<ShaderType, IDxcBlob*>(dxShader->m_ShaderBlobs, ShaderType::Vertex, nullptr);
		const ComPtr<IDxcBlob> pixelShader = eastl::try_at<ShaderType, IDxcBlob*>(dxShader->m_ShaderBlobs, ShaderType::Pixel, nullptr);
		const ComPtr<IDxcBlob> vertexReflection = eastl::try_at<ShaderType, IDxcBlob*>(dxShader->m_ReflectionBlobs, ShaderType::Vertex, nullptr);
		const ComPtr<IDxcBlob> pixelReflection = eastl::try_at<ShaderType, IDxcBlob*>(dxShader->m_ReflectionBlobs, ShaderType::Pixel, nullptr);

		if (!vertexShader)
		{
			ARC_CORE_ERROR("Failed to create PSO for shader: {}", dxShader->GetName());
			return;
		}

		ComPtr<IDxcUtils> utils = nullptr;
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)), "DxcUtils creation failed!");

		_bstr_t shaderName = ToWCSTR(shader->GetName().c_str());
		struct Layout
		{
			eastl::string Name;
			uint32_t Index;
			DXGI_FORMAT Format;

			Layout(const char* name, uint32_t index, DXGI_FORMAT format)
				: Name(name), Index(index), Format(format)
			{
			}
		};
		eastl::vector<Layout> inputLayout;
		eastl::vector<Layout> outputLayout;
		eastl::vector<D3D12_ROOT_PARAMETER1> rootParams;
		eastl::array<D3D12_DESCRIPTOR_RANGE1, 20> rootDescriptors;
		uint32_t rootDescriptorsEnd = 0;

		if (vertexReflection)
		{
			// Create reflection interface.
			DxcBuffer reflectionData{};
			reflectionData.Encoding = DXC_CP_ACP;
			reflectionData.Ptr = vertexReflection->GetBufferPointer();
			reflectionData.Size = vertexReflection->GetBufferSize();

			ComPtr<ID3D12ShaderReflection> reflect;
			utils->CreateReflection(&reflectionData, IID_PPV_ARGS(&reflect));

			// Use reflection interface here.
			D3D12_SHADER_DESC shaderDesc;
			reflect->GetDesc(&shaderDesc);

			// Create InputLayout from reflection
			for (uint32_t i = 0; i < shaderDesc.InputParameters; ++i)
			{
				D3D12_SIGNATURE_PARAMETER_DESC desc;
				reflect->GetInputParameterDesc(i, &desc);

				DXGI_FORMAT format = GetFormatFromMaskComponents(desc.Mask, desc.ComponentType);
				if (format == DXGI_FORMAT_UNKNOWN)
				{
					ARC_CORE_ERROR("Unknown format for SemanticName: {}, SemanticIndex: {}", desc.SemanticName, desc.SemanticIndex);
					return;
				}
				inputLayout.emplace_back(desc.SemanticName, desc.SemanticIndex, format);
			}

			// Get root parameters from shader reflection data.
			AppendMaterials(reflect, rootParams, rootDescriptors, &rootDescriptorsEnd, m_MaterialProperties, m_BufferMap);
		}

		if (pixelReflection)
		{
			// Create reflection interface.
			DxcBuffer reflectionData{};
			reflectionData.Encoding = DXC_CP_ACP;
			reflectionData.Ptr = pixelReflection->GetBufferPointer();
			reflectionData.Size = pixelReflection->GetBufferSize();

			ComPtr<ID3D12ShaderReflection> reflect;
			utils->CreateReflection(&reflectionData, IID_PPV_ARGS(&reflect));

			// Use reflection interface here.
			D3D12_SHADER_DESC shaderDesc;
			reflect->GetDesc(&shaderDesc);

			if (graphicsSpec.OutputFormats.empty())
			{
				ARC_CORE_WARN("Generating output layout from reflection as none was provided to the pipeline: {}", shader->GetName());
				for (uint32_t i = 0; i < shaderDesc.OutputParameters; ++i)
				{
					D3D12_SIGNATURE_PARAMETER_DESC desc;
					reflect->GetOutputParameterDesc(i, &desc);

					DXGI_FORMAT format = GetFormatFromMaskComponents(desc.Mask, desc.ComponentType);
					if (format == DXGI_FORMAT_UNKNOWN)
					{
						ARC_CORE_ERROR("Unknown format for SemanticName: {}, SemanticIndex: {}", desc.SemanticName, desc.SemanticIndex);
						return;
					}
					outputLayout.emplace_back(desc.SemanticName, desc.SemanticIndex, format);
				}
			}

			AppendMaterials(reflect, rootParams, rootDescriptors, &rootDescriptorsEnd, m_MaterialProperties, m_BufferMap);
		}

		for (const auto& [name, slot] : m_BufferMap)
		{
			m_CrcBufferMap.emplace(CRC32_Runtime(name), slot);
		}

		eastl::vector<D3D12_INPUT_ELEMENT_DESC> psoInputLayout;
		psoInputLayout.reserve(inputLayout.size());
		for (Layout& i : inputLayout)
		{
			constexpr D3D12_INPUT_CLASSIFICATION classification = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			const D3D12_INPUT_ELEMENT_DESC desc{ i.Name.c_str(), i.Index, i.Format, 0, D3D12_APPEND_ALIGNED_ELEMENT, classification, 0 };
			psoInputLayout.push_back(desc);
		}


		constexpr uint32_t numSamplers = 3;
		CD3DX12_STATIC_SAMPLER_DESC samplers[numSamplers];
		samplers[0].Init(0, D3D12_FILTER_ANISOTROPIC);
		samplers[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		samplers[2].Init(2, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

		// Create root signature.
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc
		{
			.Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
			.Desc_1_1
			{
				.NumParameters = static_cast<uint32_t>(rootParams.size()),
				.pParameters = rootParams.data(),
				.NumStaticSamplers = numSamplers,
				.pStaticSamplers = samplers,
				.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED  // Required for bindless
					| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
					| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
					| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
					| D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
					| D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
			}
		};

		ComPtr<ID3DBlob> rootBlob;
		ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSigDesc, &rootBlob, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob)
				ARC_CORE_ERROR("Failed to serialize Root Signature. Error: {}", static_cast<const char*>(errorBlob->GetBufferPointer()));

			return;
		}

		hr = Dx12Context::GetDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
		if (FAILED(hr))
		{
			ARC_CORE_ERROR("Failed to create Root Signature. Shader: {}", shader->GetName());
			return;
		}
		m_RootSignature->SetName(shaderName);












		// PSO /////////////////////////////////////////////////////////////////////
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_RootSignature;
		if (vertexShader)
		{
			psoDesc.VS.pShaderBytecode = vertexShader->GetBufferPointer();
			psoDesc.VS.BytecodeLength = vertexShader->GetBufferSize();
		}
		if (pixelShader)
		{
			psoDesc.PS.pShaderBytecode = pixelShader->GetBufferPointer();
			psoDesc.PS.BytecodeLength = pixelShader->GetBufferSize();
		}

		// BlendState
		D3D12_RENDER_TARGET_BLEND_DESC blendDesc;
		blendDesc.BlendEnable = TRUE;
		blendDesc.LogicOpEnable = FALSE;
		blendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
		psoDesc.BlendState.IndependentBlendEnable = FALSE;
		for (D3D12_RENDER_TARGET_BLEND_DESC& renderTargetDesc : psoDesc.BlendState.RenderTarget)
			renderTargetDesc = blendDesc;

		// RasterizerState
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		switch (graphicsSpec.CullMode)
		{
			case CullModeType::None:		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; break;
			case CullModeType::Back:		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK; break;
			case CullModeType::Front:		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT; break;
		}
		switch (graphicsSpec.FillMode)
		{
			case FillModeType::Solid:		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; break;
			case FillModeType::Wireframe:	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME; break;
		}

		// DepthStencilState
		psoDesc.DepthStencilState.StencilEnable = FALSE;

		// InputLayout
		psoDesc.InputLayout.NumElements = static_cast<uint32_t>(psoInputLayout.size());
		psoDesc.InputLayout.pInputElementDescs = psoInputLayout.data();

		psoDesc.SampleMask = 0xFFFFFFFF;
		switch (graphicsSpec.Primitive)
		{
			case PrimitiveType::Triangle:	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; break;
			case PrimitiveType::Line:		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE; break;
			case PrimitiveType::Point:		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT; break;
		}

		if (Framebuffer::IsDepthFormat(graphicsSpec.DepthFormat))
		{
			psoDesc.DSVFormat = Dx12Framebuffer::GetDxgiFormat(graphicsSpec.DepthFormat);
			psoDesc.DepthStencilState.DepthEnable = true;
		}
		else
		{
			psoDesc.DepthStencilState.DepthEnable = false;
		}
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

		switch (graphicsSpec.DepthFunc)
		{
			case DepthFuncType::None:			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NONE; break;
			case DepthFuncType::Never:			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NEVER; break;
			case DepthFuncType::Less:			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; break;
			case DepthFuncType::Equal:			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL; break;
			case DepthFuncType::LessEqual:		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; break;
			case DepthFuncType::Greater:		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER; break;
			case DepthFuncType::NotEqual:		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL; break;
			case DepthFuncType::GreaterEqual:	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL; break;
			case DepthFuncType::Always:			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS; break;
		}

		if (!outputLayout.empty())
		{
			psoDesc.NumRenderTargets = static_cast<uint32_t>(outputLayout.size());
			for (size_t i = 0; i < outputLayout.size(); ++i)
				psoDesc.RTVFormats[i] = outputLayout[i].Format;
		}
		else
		{
			const uint32_t numRenderTargets = static_cast<uint32_t>(graphicsSpec.OutputFormats.size());
			psoDesc.NumRenderTargets = numRenderTargets;
			for (size_t i = 0; i < numRenderTargets; ++i)
				psoDesc.RTVFormats[i] = Dx12Framebuffer::GetDxgiFormat(graphicsSpec.OutputFormats[i]);
		}
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;
		psoDesc.NodeMask = 0;

		hr = Dx12Context::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState));
		if (FAILED(hr))
		{
			ARC_CORE_ERROR("Failed to create Pipeline State. Shader: {}", shader->GetName());
		}

		if (m_PipelineState)
			m_PipelineState->SetName(shaderName);
	}

	void Dx12PipelineState::MakeComputePipeline(const Ref<Shader>& shader)
	{
		using Microsoft::WRL::ComPtr;

		const Dx12Shader* dxShader = reinterpret_cast<const Dx12Shader*>(shader.get());
		const ComPtr<IDxcBlob> computeShader = dxShader->m_ShaderBlobs.at(ShaderType::Compute);
		const ComPtr<IDxcBlob> computeReflection = dxShader->m_ReflectionBlobs.at(ShaderType::Compute);

		if (!computeShader)
		{
			ARC_CORE_ERROR("Failed to create PSO for shader: {}", dxShader->GetName());
			return;
		}

		ComPtr<IDxcUtils> utils = nullptr;
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)), "DxcUtils creation failed!");

		_bstr_t shaderName = ToWCSTR(shader->GetName().c_str());
		eastl::vector<D3D12_ROOT_PARAMETER1> rootParams;
		eastl::array<D3D12_DESCRIPTOR_RANGE1, 20> rootDescriptors;
		uint32_t rootDescriptorsEnd = 0;

		if (computeReflection)
		{
			// Create reflection interface.
			DxcBuffer reflectionData{};
			reflectionData.Encoding = DXC_CP_ACP;
			reflectionData.Ptr = computeReflection->GetBufferPointer();
			reflectionData.Size = computeReflection->GetBufferSize();

			ComPtr<ID3D12ShaderReflection> reflect;
			utils->CreateReflection(&reflectionData, IID_PPV_ARGS(&reflect));

			// Use reflection interface here.
			D3D12_SHADER_DESC shaderDesc;
			reflect->GetDesc(&shaderDesc);

			// Get root parameters from shader reflection data.
			AppendMaterials(reflect, rootParams, rootDescriptors, &rootDescriptorsEnd, m_MaterialProperties, m_BufferMap);
		}

		for (const auto& [name, slot] : m_BufferMap)
		{
			m_CrcBufferMap.emplace(CRC32_Runtime(name), slot);
		}

		constexpr uint32_t numSamplers = 3;
		CD3DX12_STATIC_SAMPLER_DESC samplers[numSamplers];
		samplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
		samplers[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		samplers[2].Init(2, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

		// Create root signature.
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc
		{
			.Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
			.Desc_1_1
			{
				.NumParameters = static_cast<uint32_t>(rootParams.size()),
				.pParameters = rootParams.data(),
				.NumStaticSamplers = numSamplers,
				.pStaticSamplers = samplers,
				.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE
			}
		};

		ComPtr<ID3DBlob> rootBlob;
		ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSigDesc, &rootBlob, &errorBlob);
		if (FAILED(hr))
		{
			if (errorBlob)
				ARC_CORE_ERROR("Failed to serialize Root Signature. Error: {}", static_cast<const char*>(errorBlob->GetBufferPointer()));

			return;
		}

		auto* device = Dx12Context::GetDevice();

		hr = device->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
		if (FAILED(hr))
		{
			ARC_CORE_ERROR("Failed to create Root Signature. Shader: {}", shader->GetName());
			return;
		}
		m_RootSignature->SetName(shaderName);

		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_RootSignature;
		psoDesc.CS.pShaderBytecode = computeShader->GetBufferPointer();
		psoDesc.CS.BytecodeLength = computeShader->GetBufferSize();

		hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState));
		if (FAILED(hr))
		{
			ARC_CORE_ERROR("Failed to create Pipeline State. Shader: {}", shader->GetName());
		}

		if (m_PipelineState)
			m_PipelineState->SetName(shaderName);
	}

	void Dx12PipelineState::RegisterCB(uint32_t crc, uint32_t size)
	{
		ARC_PROFILE_SCOPE();

		eastl::hash_map<uint32_t, uint32_t>::iterator it = m_CrcBufferMap.find(crc);
		if (it == m_CrcBufferMap.end())
		{
			ARC_CORE_ERROR("Failed to register constant buffer with CRC {}, Not found!", crc);
			return;
		}

		if (m_ConstantBufferMap.find(crc) != m_ConstantBufferMap.end())
			return;

		ConstantBuffer& cb = m_ConstantBufferMap[crc];

		cb.RegisterIndex = it->second;
		cb.AlignedSize = (size + 255) & ~255;

		const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(cb.AlignedSize);

		for (uint32_t i = 0; i < Dx12Context::FrameCount; ++i)
		{
			Dx12Allocator::CreateBufferResource(D3D12_HEAP_TYPE_UPLOAD, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &(cb.Allocation[i]));

			D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
			desc.BufferLocation = cb.Allocation[i]->GetResource()->GetGPUVirtualAddress();
			desc.SizeInBytes = cb.AlignedSize;
			cb.Handle[i] = Dx12Context::GetSrvHeap()->Allocate();
			Dx12Context::GetDevice()->CreateConstantBufferView(&desc, cb.Handle[i].CPU);
		}
	}

	void Dx12PipelineState::RegisterSB(uint32_t crc, uint32_t stride, uint32_t count)
	{
		ARC_PROFILE_SCOPE();

		eastl::hash_map<uint32_t, uint32_t>::iterator it = m_CrcBufferMap.find(crc);
		if (it == m_CrcBufferMap.end())
		{
			ARC_CORE_ERROR("Failed to register structured buffer with CRC {}, Not found!", crc);
			return;
		}

		if (m_StructuredBufferMap.find(crc) != m_StructuredBufferMap.end())
			return;

		uint32_t slot = it->second;
		StructuredBuffer& sb = m_StructuredBufferMap[crc];
		
		sb.RegisterIndex = slot;
		sb.Stride = stride;
		sb.Count = count;

		const uint32_t size = stride * count;
		const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(size);

		for (uint32_t i = 0; i < Dx12Context::FrameCount; ++i)
		{
			Dx12Allocator::CreateBufferResource(D3D12_HEAP_TYPE_UPLOAD, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &(sb.Allocation[i]));

			D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			desc.Buffer.StructureByteStride = sb.Stride;
			desc.Buffer.NumElements = sb.Count;
			desc.Buffer.FirstElement = 0;
			desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			sb.Handle[i] = Dx12Context::GetSrvHeap()->Allocate();
			Dx12Context::GetDevice()->CreateShaderResourceView(sb.Allocation[i]->GetResource(), &desc, sb.Handle[i].CPU);
		}
	}

	void Dx12PipelineState::BindCB(GraphicsCommandList commandList, uint32_t crc)
	{
		ARC_PROFILE_SCOPE();

		auto it = m_ConstantBufferMap.find(crc);
		if (it == m_ConstantBufferMap.end())
		{
			ARC_CORE_ERROR("Failed to find CRC: ({}) to bind a Constant Buffer", crc);
			return;
		}

		ConstantBuffer& cb = it->second;

		const D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress = cb.Allocation[Dx12Context::GetCurrentFrameIndex()]->GetResource()->GetGPUVirtualAddress();
		reinterpret_cast<D3D12GraphicsCommandList*>(commandList)->SetGraphicsRootConstantBufferView(cb.RegisterIndex, gpuVirtualAddress);
	}

	void Dx12PipelineState::BindSB(GraphicsCommandList commandList, uint32_t crc)
	{
		ARC_PROFILE_SCOPE();

		auto it = m_StructuredBufferMap.find(crc);
		if (it == m_StructuredBufferMap.end())
		{
			ARC_CORE_ERROR("Failed to find CRC: ({}) to bind a Structured Buffer", crc);
			return;
		}

		StructuredBuffer& sb = it->second;
		reinterpret_cast<D3D12GraphicsCommandList*>(commandList)->SetGraphicsRootDescriptorTable(sb.RegisterIndex, sb.Handle[Dx12Context::GetCurrentFrameIndex()].GPU);
	}
	
	void Dx12PipelineState::SetCBData(GraphicsCommandList commandList, uint32_t crc, const void* data, uint32_t size, uint32_t offset)
	{
		ARC_PROFILE_SCOPE();

		auto it = m_ConstantBufferMap.find(crc);
		if (it == m_ConstantBufferMap.end())
		{
			ARC_CORE_ERROR("Failed to find CRC: ({}) to set data in a Constant Buffer", crc);
			return;
		}

		ConstantBuffer& cb = it->second;

		D3D12MA::Allocation* allocation = cb.Allocation[Dx12Context::GetCurrentFrameIndex()];
		const D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress = allocation->GetResource()->GetGPUVirtualAddress();
		reinterpret_cast<D3D12GraphicsCommandList*>(commandList)->SetGraphicsRootConstantBufferView(cb.RegisterIndex, gpuVirtualAddress);
		Dx12Utils::SetBufferData(allocation, data, size == 0 ? cb.AlignedSize : size, offset);
	}
	
	void Dx12PipelineState::SetSBData(GraphicsCommandList commandList, uint32_t crc, const void* data, uint32_t size, uint32_t index)
	{
		ARC_PROFILE_SCOPE();

		auto it = m_StructuredBufferMap.find(crc);
		if (it == m_StructuredBufferMap.end())
		{
			ARC_CORE_ERROR("Failed to find CRC: ({}) to set data in a Structured Buffer", crc);
			return;
		}

		StructuredBuffer& sb = it->second;

		ARC_CORE_ASSERT(sb.Count > index, "Structured buffer index can't be greater than count! Overflow!");
		uint32_t currentFrameIndex = Dx12Context::GetCurrentFrameIndex();
		reinterpret_cast<D3D12GraphicsCommandList*>(commandList)->SetGraphicsRootDescriptorTable(sb.RegisterIndex, sb.Handle[currentFrameIndex].GPU);
		Dx12Utils::SetBufferData(sb.Allocation[currentFrameIndex], data, size == 0 ? sb.Stride * sb.Count : size, sb.Stride * index);
	}
}
