#include "shared.hlsl"

struct VSInBasic
{
    float3 pos : Position;
    float3 normal : Normal;
};

struct VSOutBasic
{
	float4 pos	:	SV_POSITION;
    float3 normal : Normal;
    float3 viewpos : Position;
};

struct VSIn_Color
{
    float4 color : Color;
    float3 pos : Position;
    float3 normal : Normal;
};

struct VSOut_Color
{
    float4 pos : SV_POSITION;
    float3 normal : Normal;
    float3 viewpos : Position;
    float4 color : Color;
};

struct VSOut_Env
{
    float4 pos : SV_Position;
    float3 localpos : Position;
    float4 clippos : Position2;
};

VSOutBasic main(VSInBasic input)
{
    VSOutBasic output = (VSOutBasic) 0;

    output.viewpos = mul(float4(input.pos, 1.f), g_matWorldView);
    output.pos = mul(float4(input.pos, 1.f), g_matTransform);
    output.normal = mul(input.normal, (float3x3) g_matWorldView);

    return output;
}

VSOut_Color VS_Cone(VSIn_Color input)
{
    VSOut_Color output = (VSOut_Color) 0;

    output.viewpos = mul(float4(input.pos, 1.f), g_matWorldView);
    output.pos = mul(float4(input.pos, 1.f), g_matTransform);
    output.normal = mul(input.normal, (float3x3) g_matWorldView);
    output.color = input.color;

	return output;
}

float4 VS(float3 pos    :   Position)   :   SV_Position
{
    return mul(float4(pos, 1.f), g_matTransform);
}

VSOut VS_FLUID(VSStandardIn input, uint i : SV_VertexID)
{
    VSOut output;
    
    float4 pos = float4(input.pos.x, g_vecCurrentHeightField[i], input.pos.z, 1.f);
    
    output.pos = mul(pos, g_matTransform);
    output.view = mul(pos, g_matWorldView);
    output.clip = output.pos;
    
    float3 tangent = normalize(float3(2 * g_fFluidDist, 0.f, g_vecCurrentHeightField[i + 1] - g_vecCurrentHeightField[i - 1]));
    float3 bitangent = normalize(float3(0.f, -2 * g_fFluidDist, g_vecCurrentHeightField[i + g_iFluidWidth] - g_vecCurrentHeightField[i - g_iFluidWidth]));
    float3 normal = normalize(cross(tangent, bitangent));
    
    output.normal = normalize(mul(normal, (float3x3) g_matWorldView));
    output.tangent.xyz = normalize(mul(tangent, (float3x3) g_matWorldView));
    output.tangent.w = 1.f;
    
    return output;
}

VSOut_Env VS_ENV(VSStandardIn input)
{
    VSOut_Env output;
    
    output.pos = mul(float4(input.pos, 1.f), g_matTransform);
    output.localpos = input.pos;
    output.clippos = output.pos;    
    
    return output;
}
