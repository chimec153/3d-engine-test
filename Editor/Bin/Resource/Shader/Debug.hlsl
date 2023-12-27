#include "shared.hlsl"

struct VSOutDebug
{
    float4 pos  :   SV_POSITION;
    float2 pos2   :   POSITION;
};

static const float2 g_position[4] =
{
    float2(0.f, 1.f),
    float2(1.f, 1.f),
    float2(0.f, 0.f),
    float2(1.f, 0.f),
};

VSOutDebug NullVS(uint index : SV_VertexID)
{
    VSOutDebug output;
    
    output.pos = mul(float4(g_position[index], 0.f, 1.f), g_matTransform);
    output.pos2 = float2(g_position[index].x, 1.f - g_position[index].y);

    return output;
}

float4 NullPS(VSOutDebug input) : SV_TARGET
{
    return g_Texture.Sample(g_sPoint, input.pos2);
}

float4 CollideDebugPS(VSMultiOut input) :   SV_TARGET
{    
    return g_vDiffuseColor;
}

PSOut DebugPS(VSOut input)
{
    PSOut output;
    
    output.value0 = g_vDiffuseColor;
    output.value1 = float4(input.normal * 0.5f + 0.5f, 1.f);
    output.value2 = 1.f;
    output.value3 = 1.f;
    
    return output;
}

float4 DebugAlphaPS(VSOut input)    :   SV_Target
{    
    return g_vDiffuseColor;
}