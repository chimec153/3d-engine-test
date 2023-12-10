#include "shared.hlsl"

struct VS_In
{
    float4 tangent  :   Tangent;
    float3 pos  :   Position;
    float3 normal   :   Normal;
    float2 uv   :   Texcoord;
};

struct VS_Out
{
    float4 pos  :   SV_Position;
    float2 uv   :   Texcoord;
    float3 light    :   Light;
    float3 view : View;
    float3 lightDir : LightDir;
};

VS_Out VS(VS_In input)
{
    VS_Out output = (VS_Out)0.f;

    output.pos = mul(float4(input.pos, 1.f), g_matTransform);
    output.uv = input.uv;
    
    float3 viewPos = mul(float4(input.pos, 1.f), g_matWorldView).xyz;
    float3 viewNormal = normalize(mul(input.normal, (float3x3) g_matWorldView));
    float3 viewTangent = normalize(mul(input.tangent.xyz, (float3x3) g_matWorldView));
    float3 viewBitangent = cross(viewNormal, viewTangent) * input.tangent.w;
    
    float3 vPointToLight = 0.f;
    float3 lightDir = 0.f;
    
    if (g_iLightType == POINT_LIGHT)
    {
        vPointToLight = g_vLightPos - viewPos;
    }
    else if (g_iLightType == SPOT_LIGHT)
    {
        vPointToLight = g_vLightPos - viewPos;
        
        lightDir = -g_vLightDir;
        
        output.lightDir = normalize(float3(dot(lightDir, viewTangent), dot(lightDir, viewBitangent), dot(lightDir, viewNormal)));
    }
    else if(g_iLightType == DIRECTIONAL_LIGHT)
    {
        vPointToLight = -g_vLightDir;
    }
    
    output.light = float3(dot(vPointToLight, viewTangent), dot(vPointToLight, viewBitangent), dot(vPointToLight, viewNormal));
    output.view = normalize(float3(dot(-viewPos, viewTangent), dot(-viewPos, viewBitangent), dot(-viewPos, viewNormal)));

    return output;
}

float4 PS(VS_Out input) : SV_Target
{
    float4 T = g_Texture.Sample(g_sPoint, input.uv);
    float4 S = g_SpecularTexture.Sample(g_sPoint, input.uv);
    float4 E = g_EmissiveTexture.Sample(g_sPoint, input.uv);
    
    float3 normal = g_NormalTexture.Sample(g_sPoint, input.uv).xyz;
    
    normal.xy = normal.xy * 2.f - 1.f;
    normal.z = normal.z;
    
    normal = normalize(normal);
    
    float4 vAtt = 0.f;
    float3 light = 0.f;
    
    if (g_iLightType == POINT_LIGHT)
    {
        vAtt = GetLightAtt(input.light);
    
        light = normalize(input.light);
    }
    else if (g_iLightType == SPOT_LIGHT)
    {
        light = normalize(input.light);
        
        vAtt = GetLightAtt(input.light) * pow(max(dot(light, input.lightDir), 0.f), g_fLightIntensity);
    }
    else if (g_iLightType == DIRECTIONAL_LIGHT)
    {
        light = normalize(input.light);
        
        vAtt = g_vLightColor;
    }
    
    float3 hdir = normalize(light + input.view);
    
    return g_vEmissiveColor * E 
    + g_vDiffuseColor * T * g_vAmbientColor 
    + vAtt * (g_vDiffuseColor * T * max(dot(normal, light), 0.f)
    + g_vSpecularColor * pow(max(dot(hdir, normal), 0), S.r * g_fMaterialSpecPower) * (dot(normal, light) > 0));
}

float4 PS_NoNormal(VS_Out input) : SV_Target
{
    float4 T = g_Texture.Sample(g_sPoint, input.uv);
    float4 S = g_SpecularTexture.Sample(g_sPoint, input.uv);
    float4 E = g_EmissiveTexture.Sample(g_sPoint, input.uv);
    
    float3 normal = float3(0.f, 0.f, 1.f);
    
    float4 vAtt = 0.f;
    float3 light = 0.f;
    
    if (g_iLightType == POINT_LIGHT)
    {
        vAtt = GetLightAtt(input.light);
    
        light = normalize(input.light);
    }
    else if (g_iLightType == SPOT_LIGHT)
    {
        light = normalize(input.light);
        
        vAtt = GetLightAtt(input.light) * pow(max(dot(light, -g_vLightDir), 0.f), g_fLightIntensity);
    }
    else if (g_iLightType == DIRECTIONAL_LIGHT)
    {
        light = normalize(input.light);
        
        vAtt = g_vLightColor;
    }
    
    float3 hdir = normalize(light + input.view);
    
    return g_vEmissiveColor * E + g_vDiffuseColor * T * g_vAmbientColor + vAtt * (g_vDiffuseColor * T * dot(normal, light) + g_vSpecularColor * S * pow(max(dot(hdir, normal), 0), g_fMaterialSpecPower) * (dot(normal, light) > 0));
}

float4 PS_NoNormalNoSpec(VS_Out input) : SV_Target
{
    float4 T = g_Texture.Sample(g_sPoint, input.uv);
    float4 S = 1.f;
    float4 E = g_EmissiveTexture.Sample(g_sPoint, input.uv);
    
    float3 normal = float3(0.f, 0.f, 1.f);
    
    float4 vAtt = 0.f;
    float3 light = 0.f;
    
    if (g_iLightType == POINT_LIGHT)
    {
        vAtt = GetLightAtt(input.light);
    
        light = normalize(input.light);
    }
    else if (g_iLightType == SPOT_LIGHT)
    {
        light = normalize(input.light);
        
        vAtt = GetLightAtt(input.light) * pow(max(dot(light, -g_vLightDir), 0.f), g_fLightIntensity);
    }
    else if (g_iLightType == DIRECTIONAL_LIGHT)
    {
        light = normalize(input.light);
        
        vAtt = g_vLightColor;
    }
    
    float3 hdir = normalize(light + input.view);
    
    float4 vDiffuseTerm = g_vDiffuseColor * T * dot(normal, light);
    
    vDiffuseTerm = clamp(vDiffuseTerm, (float4) (0.f), (float4) (1.f));
    
    return g_vEmissiveColor * E + g_vDiffuseColor * T * g_vAmbientColor + vAtt * (vDiffuseTerm + g_vSpecularColor * S * pow(max(dot(hdir, normal), 0), g_fMaterialSpecPower) * (dot(normal, light) > 0));
}

float4 PS_NoSpec(VS_Out input) : SV_Target
{
    float4 T = g_Texture.Sample(g_sPoint, input.uv);
    float4 S = 1.f;
    float4 E = g_EmissiveTexture.Sample(g_sPoint, input.uv);
    
    float3 normal =  g_NormalTexture.Sample(g_sPoint, input.uv).xyz;
    //float3 normal = { 0.5f, 0.5f, 1.f };
    
    //normal.xy = normal.xy * 2.f - 1.f;
    //normal.z = normal.z;
    
    normal = normalize(normal);
    
    float4 vAtt = 0.f;
    float3 light = 0.f;
    
    if (g_iLightType == POINT_LIGHT)
    {
        vAtt = GetLightAtt(input.light);
    
        light = normalize(input.light);
    }
    else if (g_iLightType == SPOT_LIGHT)
    {
        light = normalize(input.light);
        
        vAtt = GetLightAtt(input.light) * pow(max(dot(light, -g_vLightDir), 0.f), g_fLightIntensity);
    }
    else if (g_iLightType == DIRECTIONAL_LIGHT)
    {
        light = normalize(input.light);
        
        vAtt = g_vLightColor;
    }
    
    float3 hdir = normalize(light + input.view);
    
    return g_vEmissiveColor * E + g_vDiffuseColor * T * g_vAmbientColor + vAtt * (g_vDiffuseColor * T * dot(normal, light) + g_vSpecularColor * S * pow(max(dot(hdir, normal), 0), g_fMaterialSpecPower) * (dot(normal, light) > 0.f));
}


float4 PS_NoDiffuseNoNormalNoSpec(VS_Out input) : SV_Target
{
    float4 E = g_EmissiveTexture.Sample(g_sPoint, input.uv);
    
    float3 normal = float3(0.f, 0.f, 1.f);
    
    float4 vAtt = 0.f;
    float3 light = 0.f;
    
    if (g_iLightType == POINT_LIGHT)
    {
        vAtt = GetLightAtt(input.light);
    
        light = normalize(input.light);
    }
    else if (g_iLightType == SPOT_LIGHT)
    {
        light = normalize(input.light);
        
        vAtt = GetLightAtt(input.light) * pow(max(dot(light, -g_vLightDir), 0.f), g_fLightIntensity);
    }
    else if (g_iLightType == DIRECTIONAL_LIGHT)
    {
        light = normalize(input.light);
        
        vAtt = g_vLightColor;
    }
    
    float3 hdir = normalize(light + input.view);
    
    return g_vEmissiveColor * E + g_vDiffuseColor * g_vAmbientColor + vAtt * (g_vDiffuseColor * dot(normal, light) + g_vSpecularColor * pow(max(dot(hdir, normal), 0), g_fMaterialSpecPower) * (dot(normal, light) > 0));
}