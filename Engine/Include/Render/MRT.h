#pragma once
#include "../Core/Ref.h"
#include "../Core/Ptr.h"
namespace Engine
{
    class ENGINE_DLL MRT :
        public CRef
    {
    public:
        // eStencilSRVFormat: DXGI_FORMAT_UNKNOWN(기본)이면 stencil SRV 미생성.
        // CustomDepth용 D24S8 타겟에서 stencil을 PS로 읽으려면
        // DXGI_FORMAT_X24_TYPELESS_G8_UINT 지정 — UE CustomStencil 패턴.
        MRT(const std::vector<DXGI_FORMAT>& format, UINT iSlot,
            DXGI_FORMAT eDepthTextureFormat = DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT eDSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT eDepthSRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
            DXGI_FORMAT eStencilSRVFormat = DXGI_FORMAT_UNKNOWN);
        virtual ~MRT() = default;

    private:
        std::vector<CPtr<ID3D11RenderTargetView>>   m_vecRTV;
        std::vector<CPtr<ID3D11ShaderResourceView>>   m_vecSRV;
        CPtr<ID3D11DepthStencilView>   m_pDSV;
        // Read-only-depth view of the same depth buffer. Lets a pass bind the
        // depth as a DSV (for depth TEST) and as an SRV (t10) at the same time
        // without the D3D11 read/write hazard — used by the alpha pass so
        // soft-particle effects (laser beams) can sample scene depth.
        CPtr<ID3D11DepthStencilView>   m_pReadOnlyDSV;
        CPtr<ID3D11ShaderResourceView>   m_pDepthSRV;
        // D24S8 타겟의 stencil 채널을 PS에서 읽기 위한 두 번째 SRV. 깊이
        // 텍스처와 같은 리소스에 X24_TYPELESS_G8_UINT 뷰만 따로 만든 것.
        CPtr<ID3D11ShaderResourceView>   m_pStencilSRV;
        std::vector<CPtr<ID3D11RenderTargetView>>   m_vecPrevRTV;
        CPtr<ID3D11DepthStencilView>   m_pPrevDSV;
        UINT m_iSlot;

    public:
        const std::vector<CPtr<ID3D11ShaderResourceView>>& GetSRVs()    const;
        CPtr<ID3D11ShaderResourceView> GetDepthSRV()    const;
        CPtr<ID3D11ShaderResourceView> GetStencilSRV()  const;
        CPtr<ID3D11DepthStencilView> GetDSV()   const;
        // May be null if the driver/format rejected a read-only-depth view;
        // callers fall back to GetDSV() in that case.
        CPtr<ID3D11DepthStencilView> GetReadOnlyDSV()   const;

    public:
        void Clear(D3D11_CLEAR_FLAG eClearFlag = (D3D11_CLEAR_FLAG)((int)D3D11_CLEAR_DEPTH | (int)D3D11_CLEAR_STENCIL));
        void SetTargets();
        void SetTargets(CPtr<ID3D11DepthStencilView> pDSV);
        void SetRenderTargets();
        void ResetTargets();
        void SetSRV();
        void SetSRV(int iIndex, UINT iSlot);
        void ResetSRV();
        void SetDepthSRV(UINT iSlot);
        // ctor에 eStencilSRVFormat을 넘긴 경우에만 의미 있음. 그 외엔 무동작.
        void SetStencilSRV(UINT iSlot);
        void ResetSRV(UINT iSlot);
    };

}