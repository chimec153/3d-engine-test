#include "D3D11Context.h"
#include "GpuResources.h"
#include <d3d11.h>

namespace Engine::RenderV2
{
	D3D11Context::D3D11Context(ID3D11Device* device, ID3D11DeviceContext* ctx)
		: m_device(device), m_ctx(ctx)
	{
	}

	void D3D11Context::SetVertexShader(GpuResource* vs)
	{
		ID3D11VertexShader* h = vs ? static_cast<VertexShaderRes*>(vs)->Handle() : nullptr;
		m_ctx->VSSetShader(h, nullptr, 0);
	}

	void D3D11Context::SetPixelShader(GpuResource* ps)
	{
		ID3D11PixelShader* h = ps ? static_cast<PixelShaderRes*>(ps)->Handle() : nullptr;
		m_ctx->PSSetShader(h, nullptr, 0);
	}

	void D3D11Context::SetInputLayout(GpuResource* layout)
	{
		ID3D11InputLayout* h = layout ? static_cast<InputLayoutRes*>(layout)->Handle() : nullptr;
		m_ctx->IASetInputLayout(h);
	}

	void D3D11Context::SetVertexBuffer(GpuResource* vb, uint32_t stride, uint32_t offset)
	{
		ID3D11Buffer* h = vb ? static_cast<VertexBufferRes*>(vb)->Handle() : nullptr;
		UINT s = stride, o = offset;
		m_ctx->IASetVertexBuffers(0, 1, &h, &s, &o);
	}

	void D3D11Context::SetIndexBuffer(GpuResource* ib, IndexFormat format)
	{
		ID3D11Buffer* h = ib ? static_cast<IndexBufferRes*>(ib)->Handle() : nullptr;
		DXGI_FORMAT fmt = (format == IndexFormat::UInt32) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
		m_ctx->IASetIndexBuffer(h, fmt, 0);
	}

	void D3D11Context::SetTopology(Topology topo)
	{
		D3D11_PRIMITIVE_TOPOLOGY t = (topo == Topology::TriangleStrip)
			? D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
			: D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		m_ctx->IASetPrimitiveTopology(t);
	}

	void D3D11Context::SetConstantBuffer(ShaderStage stage, uint32_t slot, GpuResource* cb)
	{
		ID3D11Buffer* h = cb ? static_cast<ConstantBufferRes*>(cb)->Handle() : nullptr;
		if (stage == ShaderStage::Vertex)
			m_ctx->VSSetConstantBuffers(slot, 1, &h);
		else
			m_ctx->PSSetConstantBuffers(slot, 1, &h);
	}

	void D3D11Context::UpdateConstantBuffer(GpuResource* cb, const void* data, size_t size)
	{
		auto* res = static_cast<ConstantBufferRes*>(cb);
		ID3D11Buffer* h = res->Handle();
		D3D11_MAPPED_SUBRESOURCE mapped = {};
		if (SUCCEEDED(m_ctx->Map(h, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			memcpy(mapped.pData, data, size);
			m_ctx->Unmap(h, 0);
		}
	}

	void D3D11Context::SetTexture(ShaderStage stage, uint32_t slot, GpuResource* tex)
	{
		ID3D11ShaderResourceView* h = tex ? static_cast<TextureRes*>(tex)->SRV() : nullptr;
		if (stage == ShaderStage::Vertex)
			m_ctx->VSSetShaderResources(slot, 1, &h);
		else
			m_ctx->PSSetShaderResources(slot, 1, &h);
	}

	void D3D11Context::SetSampler(ShaderStage stage, uint32_t slot, GpuResource* sampler)
	{
		ID3D11SamplerState* h = sampler ? static_cast<SamplerRes*>(sampler)->Handle() : nullptr;
		if (stage == ShaderStage::Vertex)
			m_ctx->VSSetSamplers(slot, 1, &h);
		else
			m_ctx->PSSetSamplers(slot, 1, &h);
	}

	void D3D11Context::SetStructuredBufferVS(uint32_t slot, GpuResource* buf)
	{
		ID3D11ShaderResourceView* h = buf
			? static_cast<StructuredBufferRes*>(buf)->SRV()
			: nullptr;
		m_ctx->VSSetShaderResources(slot, 1, &h);
	}

	void D3D11Context::SetBlendState(GpuResource* blend)
	{
		ID3D11BlendState* h = blend ? static_cast<BlendStateRes*>(blend)->Handle() : nullptr;
		const float factor[4] = { 1, 1, 1, 1 };
		m_ctx->OMSetBlendState(h, factor, 0xFFFFFFFF);
	}

	void D3D11Context::DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex)
	{
		m_ctx->DrawIndexed(indexCount, startIndex, baseVertex);
	}
}
