#include "shared.hlsl"

const static float4 g_vUIPosition[4] =
{
    {0.f,1.f,0.f,1.f},
    {1.f,1.f,0.f,1.f},
    {0.f,0.f,0.f,1.f},
    {1.f,0.f,0.f,1.f},
};
const static float2 g_vUIUV[4] =
{
    { 0.f, 0.f},
    { 1.f, 0.f},
    { 0.f, 1.f},
    { 1.f, 1.f},
};

VSMultiOut VS_UI(uint iVertexID :   SV_VertexID)
{
    VSMultiOut output = (VSMultiOut)0;
    
    output.pos = mul(g_vUIPosition[iVertexID], g_matTransform);
    output.uv = g_vUIUV[iVertexID] * (g_vUIEndUV - g_vUIStartUV) + g_vUIStartUV;
    
    return output;
}

float4 PS_UI(VSMultiOut input)  :   SV_TARGET
{
    return g_Texture.Sample(g_sAnisotropic, input.uv);
}