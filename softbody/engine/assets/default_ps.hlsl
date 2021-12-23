#include "lighting.hlsli"
#include "common.hlsli"

float4 main(meshshadervertex input) : SV_TARGET
{
    float4 const ambientcolor = globals.ambient * objectconstants.mat.diffuse;
    float4 const color = computelighting(globals.lights, objectconstants.mat, input.position, normalize(input.normal));

    float4 finalcolor = ambientcolor + color;
    finalcolor.a = objectconstants.mat.diffuse.a;

    return finalcolor;
}