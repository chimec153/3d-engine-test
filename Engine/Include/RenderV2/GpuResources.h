#pragma once

#include "GpuResource.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <string>

namespace Engine::RenderV2
{
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	class VertexShaderRes : public GpuResource
	{
	public:
		VertexShaderRes() : GpuResource(GpuResourceKind::VertexShader) {}

		bool LoadFromFile(ID3D11Device* device, const wchar_t* hlslPath, const char* entry);
		ID3D11VertexShader* Handle() const { return m_shader.Get(); }
		const std::vector<uint8_t>& Bytecode() const { return m_bytecode; }

	private:
		ComPtr<ID3D11VertexShader> m_shader;
		std::vector<uint8_t> m_bytecode;
	};

	class PixelShaderRes : public GpuResource
	{
	public:
		PixelShaderRes() : GpuResource(GpuResourceKind::PixelShader) {}

		bool LoadFromFile(ID3D11Device* device, const wchar_t* hlslPath, const char* entry);
		ID3D11PixelShader* Handle() const { return m_shader.Get(); }

	private:
		ComPtr<ID3D11PixelShader> m_shader;
	};

	class InputLayoutRes : public GpuResource
	{
	public:
		InputLayoutRes() : GpuResource(GpuResourceKind::InputLayout) {}

		bool Create(ID3D11Device* device,
		            const D3D11_INPUT_ELEMENT_DESC* elements, uint32_t count,
		            const void* vsBytecode, size_t vsBytecodeSize);
		ID3D11InputLayout* Handle() const { return m_layout.Get(); }

	private:
		ComPtr<ID3D11InputLayout> m_layout;
	};

	class VertexBufferRes : public GpuResource
	{
	public:
		VertexBufferRes() : GpuResource(GpuResourceKind::VertexBuffer) {}

		bool Create(ID3D11Device* device, const void* data, size_t byteSize);
		ID3D11Buffer* Handle() const { return m_buffer.Get(); }

	private:
		ComPtr<ID3D11Buffer> m_buffer;
	};

	class IndexBufferRes : public GpuResource
	{
	public:
		IndexBufferRes() : GpuResource(GpuResourceKind::IndexBuffer) {}

		bool Create(ID3D11Device* device, const void* data, size_t byteSize);
		ID3D11Buffer* Handle() const { return m_buffer.Get(); }

	private:
		ComPtr<ID3D11Buffer> m_buffer;
	};

	class ConstantBufferRes : public GpuResource
	{
	public:
		ConstantBufferRes() : GpuResource(GpuResourceKind::ConstantBuffer) {}

		bool Create(ID3D11Device* device, size_t byteSize);
		ID3D11Buffer* Handle() const { return m_buffer.Get(); }
		size_t Size() const { return m_size; }

	private:
		ComPtr<ID3D11Buffer> m_buffer;
		size_t m_size = 0;
	};

	class TextureRes : public GpuResource
	{
	public:
		TextureRes() : GpuResource(GpuResourceKind::Texture) {}

		// CreateFromMemory: builds a 2D texture from raw RGBA8 pixel data
		// (rowPitch = width * 4). Used by simple procedural cases.
		bool CreateFromMemory(ID3D11Device* device, uint32_t width, uint32_t height,
		                      const void* rgba8Pixels);

		// LoadFromFile: loads via DirectXTex. Auto-detects DDS/TGA/WIC
		// (WIC covers PNG/JPG/BMP) by file extension.
		bool LoadFromFile(ID3D11Device* device, const wchar_t* fullPath);

		ID3D11ShaderResourceView* SRV() const { return m_srv.Get(); }

	private:
		ComPtr<ID3D11Texture2D>          m_texture;
		ComPtr<ID3D11ShaderResourceView> m_srv;
	};

	class SamplerRes : public GpuResource
	{
	public:
		SamplerRes() : GpuResource(GpuResourceKind::Sampler) {}

		bool CreateLinearWrap(ID3D11Device* device);
		ID3D11SamplerState* Handle() const { return m_sampler.Get(); }

	private:
		ComPtr<ID3D11SamplerState> m_sampler;
	};

	class BlendStateRes : public GpuResource
	{
	public:
		BlendStateRes() : GpuResource(GpuResourceKind::BlendState) {}

		// Standard SrcAlpha / InvSrcAlpha for transparency.
		bool CreateAlphaBlend(ID3D11Device* device);
		ID3D11BlendState* Handle() const { return m_state.Get(); }

	private:
		ComPtr<ID3D11BlendState> m_state;
	};

	// Dynamic structured buffer (SRV-only). Used for per-frame data the VS
	// indexes — bone matrices for skinning, instance transforms, etc.
	class StructuredBufferRes : public GpuResource
	{
	public:
		StructuredBufferRes() : GpuResource(GpuResourceKind::StructuredBuffer) {}

		bool Create(ID3D11Device* device, uint32_t elementSize, uint32_t elementCount);

		// Map → memcpy → Unmap. Caller supplies element-sized data.
		void Update(ID3D11DeviceContext* ctx, const void* data, size_t bytes);

		ID3D11ShaderResourceView* SRV() const { return m_srv.Get(); }
		uint32_t ElementCount() const { return m_count; }

	private:
		ComPtr<ID3D11Buffer>             m_buffer;
		ComPtr<ID3D11ShaderResourceView> m_srv;
		uint32_t                         m_count = 0;
		uint32_t                         m_stride = 0;
	};
}
