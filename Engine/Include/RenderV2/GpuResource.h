#pragma once

#include <cstdint>

namespace Engine::RenderV2
{
	class RenderContext;

	enum class GpuResourceKind : uint8_t
	{
		VertexShader,
		PixelShader,
		InputLayout,
		VertexBuffer,
		IndexBuffer,
		ConstantBuffer,
		Texture,
		Sampler,
		BlendState,
		StructuredBuffer,
	};

	// Base for all GPU-side resources. Distinct from Component: this represents
	// data/state that lives on the GPU and binds to the pipeline. Lifecycle is
	// managed via shared_ptr by owning RenderProxy or resource cache.
	class GpuResource
	{
	public:
		explicit GpuResource(GpuResourceKind kind) : m_kind(kind) {}
		virtual ~GpuResource() = default;

		GpuResourceKind Kind() const { return m_kind; }

	private:
		GpuResourceKind m_kind;
	};
}
