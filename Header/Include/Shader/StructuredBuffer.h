#pragma once

#include "../Core/Ptr.h"

namespace Engine
{
    class ENGINE_DLL StructuredBuffer
    {
    public:
        StructuredBuffer(int iCount, int iSize, void* pData = nullptr, D3D11_USAGE eUsage = D3D11_USAGE_DEFAULT, UINT eFlag = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
        ~StructuredBuffer() = default;

    private:
        CPtr<ID3D11ShaderResourceView> m_pSRV;
        CPtr<ID3D11UnorderedAccessView> m_pUAV;
        CPtr<ID3D11Buffer>  m_pBuffer;
        CPtr<ID3D11Buffer>  m_pReadBuffer;
        CPtr<ID3D11Buffer>  m_pWriteBuffer;
        int m_iCount;
        int m_iSize;

    public:
        bool CreateBuffer(int iCount, int iSize, void* pData = nullptr, D3D11_USAGE eUsage = D3D11_USAGE_DEFAULT, UINT eFlag = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
        void SetSRV(int iSlot);
        void SetUAV(int iSlot);
        void ResetSRV(int iSlot);
        void ResetUAV(int iSlot);
        void WriteData(void* pData, int iCount);
        int GetCount()  const;
        void ReadBuffer(void* pData, int iOffset, int iSize);
        void WriteData(const void* pData, int iOffset, int iSize);
    };
}