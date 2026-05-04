#pragma once

#include <cstddef>
#include <cstdint>

namespace Engine::RenderV2
{
	class GpuResource;

	enum class ShaderStage : uint8_t
	{
		Vertex,
		Pixel,
	};

	enum class IndexFormat : uint8_t
	{
		UInt16,
		UInt32,
	};

	enum class Topology : uint8_t
	{
		TriangleList,
		TriangleStrip,
	};

	// Backend-agnostic immediate render API. GpuResource subclasses call into
	// this in their Bind(); the concrete backend (D3D11Context) translates to
	// the actual graphics API. Migrating to D3D12/Vulkan only requires a new
	// implementation of this interface.
	class RenderContext
	{
	public:
		virtual ~RenderContext() = default;

		virtual void SetVertexShader(GpuResource* vs) = 0;
		virtual void SetPixelShader(GpuResource* ps) = 0;
		virtual void SetInputLayout(GpuResource* layout) = 0;
		virtual void SetVertexBuffer(GpuResource* vb, uint32_t stride, uint32_t offset) = 0;
		virtual void SetIndexBuffer(GpuResource* ib, IndexFormat format) = 0;
		virtual void SetTopology(Topology topo) = 0;
		virtual void SetConstantBuffer(ShaderStage stage, uint32_t slot, GpuResource* cb) = 0;
		virtual void UpdateConstantBuffer(GpuResource* cb, const void* data, size_t size) = 0;

		virtual void SetTexture(ShaderStage stage, uint32_t slot, GpuResource* tex) = 0;
		virtual void SetSampler(ShaderStage stage, uint32_t slot, GpuResource* sampler) = 0;

		// Bind a StructuredBufferRes as VS SRV. Used for skinning bone
		// matrices (engine convention: t30) and instance data.
		virtual void SetStructuredBufferVS(uint32_t slot, GpuResource* buf) = 0;

		// nullptr restores default (opaque, no blend).
		virtual void SetBlendState(GpuResource* blend) = 0;

		virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) = 0;
	};
}
