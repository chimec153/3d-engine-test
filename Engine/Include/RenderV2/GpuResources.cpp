#include "GpuResources.h"
#include "../Bindable/Texture.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

namespace Engine::RenderV2
{
	static bool CompileShader(const wchar_t* path, const char* entry, const char* target,
	                          ComPtr<ID3DBlob>& outBlob)
	{
		ComPtr<ID3DBlob> err;
		UINT flags = 0;
#ifdef _DEBUG
		flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		HRESULT hr = D3DCompileFromFile(path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		                                entry, target, flags, 0, &outBlob, &err);
		return SUCCEEDED(hr);
	}

	bool VertexShaderRes::LoadFromFile(ID3D11Device* device, const wchar_t* hlslPath, const char* entry)
	{
		ComPtr<ID3DBlob> blob;
		if (!CompileShader(hlslPath, entry, "vs_5_0", blob))
			return false;

		HRESULT hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(),
		                                        nullptr, &m_shader);
		if (FAILED(hr))
			return false;

		m_bytecode.assign(reinterpret_cast<const uint8_t*>(blob->GetBufferPointer()),
		                  reinterpret_cast<const uint8_t*>(blob->GetBufferPointer()) + blob->GetBufferSize());
		return true;
	}

	bool PixelShaderRes::LoadFromFile(ID3D11Device* device, const wchar_t* hlslPath, const char* entry)
	{
		ComPtr<ID3DBlob> blob;
		if (!CompileShader(hlslPath, entry, "ps_5_0", blob))
			return false;

		HRESULT hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(),
		                                       nullptr, &m_shader);
		return SUCCEEDED(hr);
	}

	bool InputLayoutRes::Create(ID3D11Device* device,
	                            const D3D11_INPUT_ELEMENT_DESC* elements, uint32_t count,
	                            const void* vsBytecode, size_t vsBytecodeSize)
	{
		HRESULT hr = device->CreateInputLayout(elements, count, vsBytecode, vsBytecodeSize, &m_layout);
		return SUCCEEDED(hr);
	}

	static bool CreateBuffer(ID3D11Device* device, const void* data, size_t byteSize,
	                         UINT bindFlag, D3D11_USAGE usage, UINT cpuAccess,
	                         ComPtr<ID3D11Buffer>& out)
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = static_cast<UINT>(byteSize);
		desc.BindFlags = bindFlag;
		desc.Usage = usage;
		desc.CPUAccessFlags = cpuAccess;

		D3D11_SUBRESOURCE_DATA srd = {};
		srd.pSysMem = data;

		HRESULT hr = device->CreateBuffer(&desc, data ? &srd : nullptr, &out);
		return SUCCEEDED(hr);
	}

	bool VertexBufferRes::Create(ID3D11Device* device, const void* data, size_t byteSize)
	{
		return CreateBuffer(device, data, byteSize, D3D11_BIND_VERTEX_BUFFER,
		                    D3D11_USAGE_DEFAULT, 0, m_buffer);
	}

	bool IndexBufferRes::Create(ID3D11Device* device, const void* data, size_t byteSize)
	{
		return CreateBuffer(device, data, byteSize, D3D11_BIND_INDEX_BUFFER,
		                    D3D11_USAGE_DEFAULT, 0, m_buffer);
	}

	bool ConstantBufferRes::Create(ID3D11Device* device, size_t byteSize)
	{
		// CB sizes must be 16-byte aligned.
		size_t aligned = (byteSize + 15) & ~size_t(15);
		m_size = aligned;
		return CreateBuffer(device, nullptr, aligned, D3D11_BIND_CONSTANT_BUFFER,
		                    D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, m_buffer);
	}

	bool TextureRes::CreateFromMemory(ID3D11Device* device, uint32_t width, uint32_t height,
	                                  const void* rgba8Pixels)
	{
		D3D11_TEXTURE2D_DESC td = {};
		td.Width = width;
		td.Height = height;
		td.MipLevels = 1;
		td.ArraySize = 1;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_IMMUTABLE;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA srd = {};
		srd.pSysMem = rgba8Pixels;
		srd.SysMemPitch = width * 4;

		if (FAILED(device->CreateTexture2D(&td, &srd, &m_texture)))
			return false;

		D3D11_SHADER_RESOURCE_VIEW_DESC vd = {};
		vd.Format = td.Format;
		vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		vd.Texture2D.MipLevels = 1;

		return SUCCEEDED(device->CreateShaderResourceView(m_texture.Get(), &vd, &m_srv));
	}

	bool TextureRes::LoadFromFile(ID3D11Device* /*device*/, const wchar_t* fullPath)
	{
		// Delegate file IO to the engine's Texture loader (DirectXTex under
		// the hood — DDS/TGA/WIC autodetect). We grab only the SRV; the
		// Bindable lifecycle of the temporary loader is unused here.
		// Engine::CPtr and WRL::ComPtr are different smart-pointer types,
		// so we go via the raw pointer (which AddRef's on Attach).
		Texture loader;
		if (!loader.LoadTextureFromFullPath(fullPath))
			return false;

		ID3D11ShaderResourceView* raw = loader.GetSRV().Get();
		if (!raw) return false;
		m_srv.Attach(raw);
		raw->AddRef();   // CPtr destructor in `loader` will Release; keep one ref for us.
		return true;
	}

	bool SamplerRes::CreateLinearWrap(ID3D11Device* device)
	{
		D3D11_SAMPLER_DESC sd = {};
		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.MaxLOD = D3D11_FLOAT32_MAX;
		return SUCCEEDED(device->CreateSamplerState(&sd, &m_sampler));
	}

	bool BlendStateRes::CreateAlphaBlend(ID3D11Device* device)
	{
		D3D11_BLEND_DESC bd = {};
		bd.RenderTarget[0].BlendEnable = TRUE;
		bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		return SUCCEEDED(device->CreateBlendState(&bd, &m_state));
	}

	bool StructuredBufferRes::Create(ID3D11Device* device, uint32_t elementSize, uint32_t elementCount)
	{
		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = elementSize * elementCount;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bd.StructureByteStride = elementSize;

		if (FAILED(device->CreateBuffer(&bd, nullptr, &m_buffer)))
			return false;

		D3D11_SHADER_RESOURCE_VIEW_DESC vd = {};
		vd.Format = DXGI_FORMAT_UNKNOWN;
		vd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		vd.Buffer.NumElements = elementCount;

		if (FAILED(device->CreateShaderResourceView(m_buffer.Get(), &vd, &m_srv)))
			return false;

		m_count = elementCount;
		m_stride = elementSize;
		return true;
	}

	void StructuredBufferRes::Update(ID3D11DeviceContext* ctx, const void* data, size_t bytes)
	{
		D3D11_MAPPED_SUBRESOURCE mapped = {};
		if (SUCCEEDED(ctx->Map(m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			memcpy(mapped.pData, data, bytes);
			ctx->Unmap(m_buffer.Get(), 0);
		}
	}
}
