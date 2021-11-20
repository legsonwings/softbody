
#include "common.hlsli"

Texture2D tex : register(t2);
SamplerState sampler0 : register(s0);

float4 main(texturessvertex input) : SV_TARGET
{
	return tex.Sample(sampler0, input.texcoord);
}