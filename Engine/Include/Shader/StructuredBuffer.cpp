#include "StructuredBuffer.h"
#include "../Core/Graphics.h"

namespace Engine
{
	StructuredBuffer::StructuredBuffer(int iCount, int iSize, void* pData, D3D11_USAGE eUsage, UINT eFlag) :
		m_pSRV()
		, m_pUAV()
		, m_pBuffer()
		, m_iCount(iCount)
		, m_iSize(iSize)
	{
		if (!CreateBuffer(iCount, iSize, pData, eUsage, eFlag))
		{
			assert(false);
			return;
		}
	}

	bool StructuredBuffer::CreateBuffer(int iCount, int iSize, void* pData, D3D11_USAGE eUsage, UINT eFlag)
	{
		m_iCount = iCount;
		m_iSize = iSize;

		D3D11_BUFFER_DESC tSRVBufferDesc = {};

		tSRVBufferDesc.ByteWidth = iSize * iCount;
		tSRVBufferDesc.Usage = eUsage;
		tSRVBufferDesc.BindFlags = eFlag;
		tSRVBufferDesc.StructureByteStride = iSize;

		switch (eUsage)
		{
		case D3D11_USAGE_DEFAULT:
		case D3D11_USAGE_IMMUTABLE:
			tSRVBufferDesc.CPUAccessFlags = 0;
			break;
		case D3D11_USAGE_DYNAMIC:
			tSRVBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			break;
		case D3D11_USAGE_STAGING:
			tSRVBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			break;
		}

		tSRVBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

		if (pData)
		{
			D3D11_SUBRESOURCE_DATA tSubData = {};

			tSubData.pSysMem = pData;
			tSubData.SysMemPitch = iSize;

			if (FAILED(Graphics::GetInst()->GetDevice()->CreateBuffer(&tSRVBufferDesc, &tSubData, &m_pBuffer)))
			{
				assert(false);
				return false;
			}
		}
		else
		{
			if (FAILED(Graphics::GetInst()->GetDevice()->CreateBuffer(&tSRVBufferDesc, nullptr, &m_pBuffer)))
			{
				assert(false);
				return false;
			}
		}

		if (eFlag & D3D11_BIND_SHADER_RESOURCE)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC tSRVDesc = {};

			tSRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			tSRVDesc.BufferEx.NumElements = iCount;
			tSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;

			if (FAILED(Graphics::GetInst()->GetDevice()->CreateShaderResourceView(m_pBuffer.Get(), &tSRVDesc, &m_pSRV)))
			{
				assert(false);
				return false;
			}
		}

		if (eFlag & D3D11_BIND_UNORDERED_ACCESS)
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC tUAVDesc = {};

			tUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
			tUAVDesc.Buffer.NumElements = iCount;
			tUAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
			tUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

			if (FAILED(Graphics::GetInst()->GetDevice()->CreateUnorderedAccessView(m_pBuffer.Get(), &tUAVDesc, &m_pUAV)))
			{
				assert(false);
				return false;
			}
		}

		D3D11_BUFFER_DESC tDebugDesc = {};

		tDebugDesc.ByteWidth = m_iCount * iSize;
		tDebugDesc.Usage = D3D11_USAGE_STAGING;
		tDebugDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		tDebugDesc.StructureByteStride = iSize;
		tDebugDesc.BindFlags = 0;

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateBuffer(&tDebugDesc, nullptr, &m_pReadBuffer)))
		{
			assert(false);
			return false;
		}

		D3D11_BUFFER_DESC tWriteDesc = {};

		tWriteDesc.ByteWidth = iSize * iCount;
		tWriteDesc.Usage = D3D11_USAGE_DYNAMIC;
		tWriteDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		tWriteDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateBuffer(&tWriteDesc, nullptr, &m_pWriteBuffer)))
		{
			assert(false);
			return false;
		}

		return true;
	}
	void StructuredBuffer::SetSRV(int iSlot)
	{
		Graphics::GetInst()->GetDeviceContext()->VSSetShaderResources(iSlot, 1, m_pSRV.GetAdressof());
		Graphics::GetInst()->GetDeviceContext()->GSSetShaderResources(iSlot, 1, m_pSRV.GetAdressof());
		Graphics::GetInst()->GetDeviceContext()->CSSetShaderResources(iSlot, 1, m_pSRV.GetAdressof());
	}
	void StructuredBuffer::SetUAV(int iSlot)
	{
		UINT iOffset = -1;
		Graphics::GetInst()->GetDeviceContext()->CSSetUnorderedAccessViews(iSlot, 1, m_pUAV.GetAdressof(), &iOffset);
	}
	void StructuredBuffer::ResetSRV(int iSlot)
	{
		ID3D11ShaderResourceView* pSRV = nullptr;

		Graphics::GetInst()->GetDeviceContext()->VSSetShaderResources(iSlot, 1, &pSRV);
		Graphics::GetInst()->GetDeviceContext()->GSSetShaderResources(iSlot, 1, &pSRV);
		Graphics::GetInst()->GetDeviceContext()->CSSetShaderResources(iSlot, 1, &pSRV);
	}
	void StructuredBuffer::ResetUAV(int iSlot)
	{
		ID3D11UnorderedAccessView* pUAV = nullptr;

		Graphics::GetInst()->GetDeviceContext()->CSSetUnorderedAccessViews(iSlot, 1, &pUAV, nullptr);
	}
	void StructuredBuffer::WriteData(void* pData, int iCount)
	{
		D3D11_MAPPED_SUBRESOURCE tBoneSub = {};

		Graphics::GetInst()->GetDeviceContext()->Map(m_pBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &tBoneSub);

		memcpy_s(tBoneSub.pData, m_iCount * m_iSize, pData, iCount * m_iSize);

		Graphics::GetInst()->GetDeviceContext()->Unmap(m_pBuffer.Get(), 0);
	}
	int StructuredBuffer::GetCount() const
	{
		return m_iCount;
	}
	void StructuredBuffer::ReadBuffer(void* pData, int iOffset, int iSize)
	{
		Graphics::GetInst()->GetDeviceContext()->CopyResource(m_pReadBuffer.Get(), m_pBuffer.Get());

		D3D11_MAPPED_SUBRESOURCE tSub = {};
		tSub.RowPitch = m_iSize;

		if (FAILED(Graphics::GetInst()->GetDeviceContext()->Map(m_pReadBuffer.Get(), 0, D3D11_MAP::D3D11_MAP_READ, 0, &tSub)))
		{
			return;
		}

		memcpy_s(pData, iSize, reinterpret_cast<char*>(tSub.pData) + iOffset, iSize);

		Graphics::GetInst()->GetDeviceContext()->Unmap(m_pReadBuffer.Get(), 0);
	}
	void StructuredBuffer::WriteData(void* pData, int iOffset, int iSize)
	{
		D3D11_MAPPED_SUBRESOURCE tSub = {};

		Graphics::GetInst()->GetDeviceContext()->Map(m_pWriteBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &tSub);

		memcpy_s(static_cast<char*>(tSub.pData) + iOffset, m_iSize * m_iCount - iOffset, pData, iSize);

		Graphics::GetInst()->GetDeviceContext()->Unmap(m_pWriteBuffer.Get(), 0);

		Graphics::GetInst()->GetDeviceContext()->CopyResource(m_pBuffer.Get(), m_pWriteBuffer.Get());
	}
}