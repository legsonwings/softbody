#include "Common.hlsli"

float4 main(MeshShaderVertex input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 L = normalize(input.position - Globals.CamPos);

    return float4(abs(dot(N, L)) * input.color.rgb, input.color.a);
}
