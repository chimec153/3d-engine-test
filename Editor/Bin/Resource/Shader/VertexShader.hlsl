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