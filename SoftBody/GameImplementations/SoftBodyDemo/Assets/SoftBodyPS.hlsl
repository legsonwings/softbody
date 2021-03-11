#include "SoftBodyShared.hlsli"

struct VertexIn
{
    float4 PositionHS : SV_Position;
    float4 Color : COLOR0;
    float3 Position : POSITION0;
    float3 Normal : NORMAL0;
};

float4 main(VertexIn input) : SV_TARGET
{
    float3 N = normalize(input.Normal);
    float3 L = normalize(input.Position - Globals.CamPos);
    
    return abs(dot(N, L)) * input.Color;
}
