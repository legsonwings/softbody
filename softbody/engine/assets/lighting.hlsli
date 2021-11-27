#include "common.hlsli"

float attenuation(float range, float distance)
{
	return 1.f / (1.f + (distance * distance) / (range * range));
}

float3 specularcoefficient(float3 r0, float3 normal, float3 lightvec)
{
	float const oneminuscos = 1.f - saturate(dot(normal, lightvec));
	return r0 + (1 - r0) * pow(oneminuscos, 5.f);
}

float3 blinnphong(float3 toeye, float3 normal, float3 intensity, material mat, float3 lightvec)
{
	float const m = (1.f - mat.roughness) * 256.f;
	float3 const h = normalize((lightvec + toeye));

	float3 const specular = specularcoefficient(mat.fresnelr, h, lightvec);
	float3 const roughness =  (m + 8.f ) * pow(max(dot(h, normal), 0.f), m) / 8.f;
	float3 specalbedo = specular * roughness;
	
	// specalbedo can go out of range [0 1] which LDR doesn't support
	specalbedo = specalbedo / (specalbedo + 1.0f);

	return (mat.diffuse.rgb + specalbedo) * intensity;
}

float3 directionallight(light l, material m, float3 normal, float3 toeye)
{
	float3 const lightvec = -l.direction;
	float3 const lightintensity = l.color * max(dot(lightvec, normal), 0.f);
	return blinnphong(toeye, normal, lightintensity, m, lightvec);
}

float3 pointlight(light l, material m, float3 normal, float3 pos, float3 toeye)
{
	float3 lightvec = l.position - pos;
	float const d = length(lightvec);
	lightvec /= d;

	float3 const lightintensity = l.color * saturate(dot(lightvec, normal)) * attenuation(l.range, d);

	return blinnphong(toeye, normal, lightintensity, m, lightvec);
}

float4 computelighting(light lights[MAX_NUM_LIGHTS], material m, float3 pos, float3 normal)
{
	float3 result = 0;
	float3 const toeye = normalize(globals.campos - pos);
	
	int i = 0;
	for (; i < globals.numdirlights; ++i)
	{
		result += directionallight(lights[i], m, normal, toeye);
	}

	for (; i < globals.numdirlights + globals.numpointlights; ++i)
	{
		result += pointlight(lights[i], m, normal, pos, toeye);
	}

	return float4(result, 1.f);
}