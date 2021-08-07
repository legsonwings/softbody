#include "lighting.hlsli"
#include "Common.hlsli"

float4 main(MeshShaderVertex input) : SV_TARGET
{
    float4 const ambientcolor = globals.ambient * objectconstants.mat.diffusealbedo;
    float4 const color = computelighting(globals.lights, objectconstants.mat, input.position, normalize(input.normal));

    float4 finalcolor = ambientcolor + color;
    finalcolor.a = objectconstants.mat.diffusealbedo.a;

    return finalcolor;
}