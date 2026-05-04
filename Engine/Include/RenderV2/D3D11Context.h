#pragma once

#include "RenderContext.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace Engine::RenderV2
{
	// D3D11 backend for RenderContext. Wraps the engine's existing
	// ID3D11DeviceContext (obtained from Graphics::GetDeviceContext()).
	class D3D11Context : public RenderContext
	{
	public:
		D3D11Context(ID3D11Device* device, ID3D11DeviceContext* ctx);
		~D3D11Context() override = default;

		ID3D11Device* Device() const { return m_device; }
		ID3D11DeviceContext* Context() const { return m_ctx; }

		void SetVertexShader(GpuResource* vs) override;
		void SetPixelShader(GpuResource* ps) override;
		void SetInputLayout(GpuResource* layout) override;
		void SetVertexBuffer(GpuResource* vb, uint32_t stride, uint32_t offset) override;
		void SetIndexBuffer(GpuResource* ib, IndexFormat format) override;
		void SetTopology(Topology topo) override;
		void SetConstantBuffer(ShaderStage stage, uint32_t slot, GpuResource* cb) override;
		void UpdateConstantBuffer(GpuResource* cb, const void* data, size_t size) override;

		void SetTexture(ShaderStage stage, uint32_t slot, GpuResource* tex) override;
		void SetSampler(ShaderStage stage, uint32_t slot, GpuResource* sampler) override;
		void SetStructuredBufferVS(uint32_t slot, GpuResource* buf) override;
		void SetBlendState(GpuResource* blend) override;

		void DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) override;

	private:
		ID3D11Device* m_device = nullptr;
		ID3D11DeviceContext* m_ctx = nullptr;
	};
}
