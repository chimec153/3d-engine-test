#include "shared.hlsl"

struct VSInTexture
{
    float3 pos : Position;
    float3 normal : Normal;
    float2 uv : Texcoord;
};

struct VSOutTexture
{
	float4 pos	:	SV_Position;
    float2 uv : Texcoord;
    float3 normal : Normal;
    float3 viewpos : Position;
};

VSOutTexture VS(VSInTexture input)
{
    VSOutTexture output = (VSOutTexture) 0;

	output.pos = mul(float4(input.pos , 1.f), g_matTransform);
    output.uv = input.uv;
    output.normal = mul(input.normal, (float3x3) g_matWorldView);
    output.viewpos = mul(float4(input.pos, 1.f), g_matWorldView);

	return output;
}

float4 PS(VSOutTexture input) : SV_TARGET
{
    float3 vViewToPoint = normalize(input.viewpos);
    
    float3 vViewNormal = normalize(input.normal);
    
    float3 vPointToLight = g_vLightPos - vViewToPoint;
    
    float4 fAtt = GetLightAtt(vPointToLight);
    
    float4 T = g_Texture.Sample(g_sPoint, input.uv);
    float4 N = g_NormalTexture.Sample(g_sPoint, input.uv) * 2.f - 1.f;
    
    float fSpec = GetSpecular(vViewToPoint, vPointToLight, vViewNormal);
    
    return fAtt * (g_vDiffuseColor * T * max(dot(vPointToLight, vViewNormal), 0.f) + g_vSpecularColor * fSpec) + g_vDiffuseColor * T * g_vLightAmbientColor;
}