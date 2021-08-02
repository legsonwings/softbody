#include "Common.hlsli"

StructuredBuffer<instance_data> instances : register(t1);

float4 main(MeshShaderVertex input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 L = normalize(input.position - Globals.campos);

    return float4(abs(dot(N, L)) * instances[input.instanceid].mat.diffusealbedo);
}
