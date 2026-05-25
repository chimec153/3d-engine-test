// UE의 CustomDepth + CustomStencil 기반 외곽선과 동일한 패턴.
// 1) RenderCustomDepth: 외곽선 대상 메시들이 D24S8 타겟에 자기 stencil 값으로
//    REPLACE. SetCustomStencil(uint8>0) 호출한 MR만 참여.
// 2) RenderOutline (이 셰이더): 풀스크린 패스. 픽셀 stencil이 0이면 패스 통과,
//    아니면 4방향 stencil과 비교해 edge 검출 → stencil 값별 색상 LUT 적용.
//
// 결과: m_pHDRTexture(또는 backbuffer) 위에 알파블렌드로 라인만 덧칠.
//
// VS는 anisotropic_microfacet.hlsl의 VS_Multi(MultiVS)를 그대로 재사용 —
// 풀스크린 4정점 비-IL 패스.

#include "shared.hlsl"

// D24S8 텍스처의 두 채널 뷰:
//   t19: depth   (R24_UNORM_X8_TYPELESS) — MRT::GetDepthSRV() 가 제공
//   t29: stencil (X24_TYPELESS_G8_UINT)  — MRT::GetStencilSRV() 가 제공
Texture2D<float>  g_OutlineCustomDepth   : register(t19);
Texture2D<uint2>  g_OutlineCustomStencil : register(t29);

// D3D11 cbuffer 슬롯 한계가 b0..b13. shared.hlsl이 이미 b0~b12를 채우고 있고,
// b13은 UI.fx의 UITint와 공유 — Outline 패스는 RenderUI 이전에 끝나므로
// 같은 b13 슬롯에 후속 UI 바인딩이 덮어써도 무방.
cbuffer Outline : register(b13)
{
    float4 g_vOutlineColor;       // stencil == 1 일 때 사용
    float4 g_vOutlineColorAlt;    // stencil >= 2 일 때 사용
    float2 g_vOutlineTexelSize;   // 1.0/width, 1.0/height
    int    g_iOutlineThickness;   // 픽셀 단위 두께
    int    _outlinePad;
};

float4 PS_Outline(VSMultiOut input) : SV_Target
{
    int2 pixel = int2(input.pos.xy);
    int  t     = max(1, g_iOutlineThickness);

    // .y가 X24_TYPELESS_G8_UINT 뷰의 stencil 채널 — DX11 규약.
    uint sCenter = g_OutlineCustomStencil.Load(int3(pixel, 0)).y;

    // 중앙이 0이면 외곽선 그릴 대상이 아예 없음. 4방향 중에 stencil > 0 픽셀이
    // 있으면 그 픽셀의 색을 빌려 와서 "바깥쪽 라인"을 그림. (UE의 outline
    // post-process material과 동일한 양방향 검출.)
    uint sN = g_OutlineCustomStencil.Load(int3(pixel + int2(0,  t), 0)).y;
    uint sS = g_OutlineCustomStencil.Load(int3(pixel + int2(0, -t), 0)).y;
    uint sE = g_OutlineCustomStencil.Load(int3(pixel + int2( t, 0), 0)).y;
    uint sW = g_OutlineCustomStencil.Load(int3(pixel + int2(-t, 0), 0)).y;

    uint sMax = max(max(sN, sS), max(sE, sW));

    // 모든 5픽셀이 같은 stencil이면 내부 — 라인 아님.
    if (sCenter == sMax && sCenter == sN && sCenter == sS && sCenter == sE && sCenter == sW)
    {
        discard;
    }

    // 라인이 들고 갈 stencil 값: 중앙(>0)이면 중앙, 아니면 이웃 중 최대값.
    uint sLine = sCenter > 0 ? sCenter : sMax;
    if (sLine == 0) discard;

    return sLine == 1 ? g_vOutlineColor : g_vOutlineColorAlt;
}
