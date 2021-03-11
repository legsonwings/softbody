#include "..\..\SharedConstants.h"

struct Constants
{
    float3 CamPos;
    uint Padding0;
    float4x4 World;
    float4x4 WorldView;
    float4x4 WorldViewProj;
};

struct VertexIn
{
    float3 position;
    float3 normal;
};

struct MeshShaderVertex
{
    float4 projected_position : SV_Position;
    // TODO : Move this to primitive attribute instead or pass instance id to ps
    float4 color : COLOR0;
    float3 position : POSITION0;
    float3 normal : NORMAL0;
};

ConstantBuffer<Constants> Globals : register(b0);
