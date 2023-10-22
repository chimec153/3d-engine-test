#include "VertexShader.hlsl"

float4 PS(VSOutBasic input, uint id : SV_PrimitiveID) : SV_TARGET
{
    float3 vPointToLight = g_vLightPos - input.viewpos;
    
    float4 C0 = GetLightAtt(vPointToLight);
    
    float3 lightDir = normalize(vPointToLight);
    
    float3 normal = normalize(input.normal);
    
    float fSpec = GetSpecular(input.viewpos, lightDir, normal);
    
    return C0 * (g_vDiffuseColor * g_vColor[id / 2] * max(dot(normal, lightDir), 0.f) + g_vSpecularColor * fSpec) + g_vDiffuseColor * g_vLightAmbientColor * g_vColor[id / 2];
}

float4 PS_Sphere(VSOutBasic input, uint id : SV_PrimitiveID) : SV_TARGET
{
    float4 output = 0.f;
    
    float4 C0 = 0.f;
    
    float3 lightDir = 0.f;
    
    if (g_iLightType == POINT_LIGHT)
    {
        float3 vPointToLight = g_vLightPos - input.viewpos;
    
        C0 = GetLightAtt(vPointToLight);
    
        lightDir = normalize(vPointToLight);
    }
    else if (g_iLightType == SPOT_LIGHT)
    {
        float3 vPointToLight = g_vLightPos - input.viewpos;
    
        lightDir = normalize(vPointToLight);
        
        C0 = GetLightAtt(vPointToLight) * pow(max(dot(lightDir, -g_vLightDir), 0.f), g_fLightIntensity);
    }
    else
    {
        C0 = g_vLightColor;

        lightDir = -g_vLightDir;
    }
    
    float3 normal = normalize(input.normal);
    
    float fSpec = GetSpecular(normalize(input.viewpos), lightDir, normal);
    
    output += C0 * (g_vDiffuseColor * /*g_vColor[(id / 2) % 6] **/max(dot(normal, lightDir), 0.f) + g_vSpecularColor * fSpec) + g_vDiffuseColor * g_vLightAmbientColor /** g_vColor[(id / 2) % 6]*/;
    
    return output;

}

float4 PS_Cone(VSOut_Color input) : SV_TARGET
{
    float3 vPointToLight = g_vLightPos - input.viewpos;
    
    float3 normal = normalize(input.normal);
    
    float4 C0 = GetLightAtt(vPointToLight);
    
    float3 lightDir = normalize(vPointToLight);
    
    float fSpec = GetSpecular(input.viewpos, lightDir, normal);
    
    return C0 * (g_vDiffuseColor * input.color * max(dot(normal, lightDir), 0.f) + g_vSpecularColor * fSpec) + g_vDiffuseColor * g_vLightAmbientColor * input.color;
}

float4 PS_White(float4 pos  :   SV_Position) :   SV_Target
{
    return g_vLightColor;
}