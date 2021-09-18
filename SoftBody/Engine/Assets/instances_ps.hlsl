#include "lighting.hlsli"
#include "common.hlsli"

StructuredBuffer<instance_data> instances : register(t1);

float4 main(MeshShaderVertex input) : SV_TARGET
{
    float4 const ambientcolor = globals.ambient * instances[input.instanceid].mat.diffusealbedo;
    float4 const color = computelighting(globals.lights, instances[input.instanceid].mat, input.position, normalize(input.normal));

    float4 finalcolor = ambientcolor + color;
    finalcolor.a = instances[input.instanceid].mat.diffusealbedo.a;

    return finalcolor;
}