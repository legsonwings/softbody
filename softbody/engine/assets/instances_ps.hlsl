#include "lighting.hlsli"
#include "common.hlsli"

StructuredBuffer<instance_data> instances : register(t1);

float4 main(meshshadervertex input) : SV_TARGET
{
    float4 const ambientcolor = globals.ambient * instances[input.instanceid].mat.diffuse;
    float4 const color = computelighting(globals.lights, instances[input.instanceid].mat, input.position, normalize(input.normal));

    float4 finalcolor = ambientcolor + color;
    finalcolor.a = instances[input.instanceid].mat.diffuse.a;

    return finalcolor;
}