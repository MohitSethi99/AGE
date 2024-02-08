#pragma once

#include "GraphicsContext.h"

namespace ArcEngine
{
	enum class FramebufferTextureFormat : uint8_t
	{
		None = 0,

		// Color
		RGBA8,
		RGBA16F,
		RGBA32F,
		R11G11B10F,
		RG16F,
		R32I,

		// Depth/Stencil
		DEPTH24STENCIL8,

		// Defaults
		Depth = DEPTH24STENCIL8
	};

	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(FramebufferTextureFormat format)
			: TextureFormat(format) {}

		FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
		// TODO: filtering/wrap
	};

	struct FramebufferAttachmentSpecification
	{
		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
			: Attachments(attachments) {}

		eastl::vector<FramebufferTextureSpecification> Attachments;
	};

	struct FramebufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		FramebufferAttachmentSpecification Attachments;
		uint32_t Samples = 1;

		bool SwapChainTarget = false;

		eastl::string Name = "Unknown Framebuffer";
	};
	
	class Framebuffer
	{
	public:
		virtual ~Framebuffer() = default;
		
		virtual void Bind(GraphicsCommandList commandList) = 0;
		virtual void Unbind(GraphicsCommandList commandList) = 0;
		virtual void Clear(GraphicsCommandList commandList) = 0;

		virtual void BindColorAttachment(GraphicsCommandList commandList, uint32_t index, uint32_t slot) = 0;
		virtual void BindDepthAttachment(GraphicsCommandList commandList, uint32_t slot) = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		
		[[nodiscard]] virtual uint64_t GetColorAttachmentRendererID(uint32_t index = 0) const = 0;
		[[nodiscard]] virtual uint64_t GetDepthAttachmentRendererID() const = 0;
		[[nodiscard]] virtual uint32_t GetColorAttachmentHeapIndex(uint32_t index) const = 0;
		[[nodiscard]] virtual uint32_t GetDepthAttachmentHeapIndex() const = 0;
		
		[[nodiscard]] virtual const FramebufferSpecification& GetSpecification() const = 0;
		
		[[nodiscard]] static Ref<Framebuffer> Create(const FramebufferSpecification& spec);

		[[nodiscard]] static bool IsDepthFormat(FramebufferTextureFormat format)
		{
			return format == FramebufferTextureFormat::DEPTH24STENCIL8;
		}

	protected:
		static constexpr uint32_t s_MaxFramebufferSize = 8192;
	};
}
