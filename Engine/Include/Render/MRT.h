#pragma once
#include "../Core/Ref.h"
#include "../Core/Ptr.h"
namespace Engine
{
    class ENGINE_DLL MRT :
        public CRef
    {
    public:
        MRT(const std::vector<DXGI_FORMAT>& format, UINT iSlot,
            DXGI_FORMAT eDepthTextureFormat = DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT eDSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT eDepthSRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
        virtual ~MRT() = default;

    private:
        std::vector<CPtr<ID3D11RenderTargetView>>   m_vecRTV;
        std::vector<CPtr<ID3D11ShaderResourceView>>   m_vecSRV;
        CPtr<ID3D11DepthStencilView>   m_pDSV;
        CPtr<ID3D11ShaderResourceView>   m_pDepthSRV;
        std::vector<CPtr<ID3D11RenderTargetView>>   m_vecPrevRTV;
        CPtr<ID3D11DepthStencilView>   m_pPrevDSV;
        UINT m_iSlot;

    public:
        const std::vector<CPtr<ID3D11ShaderResourceView>>& GetSRVs()    const;
        CPtr<ID3D11ShaderResourceView> GetDepthSRV()    const;

    public:
        void Clear(D3D11_CLEAR_FLAG eClearFlag = (D3D11_CLEAR_FLAG)((int)D3D11_CLEAR_DEPTH | (int)D3D11_CLEAR_STENCIL));
        void SetTargets();
        void SetRenderTargets();
        void ResetTargets();
        void SetSRV();
        void SetSRV(int iIndex, UINT iSlot);
        void ResetSRV();
        void SetDepthSRV(UINT iSlot);
        void ResetSRV(UINT iSlot);
    };

}