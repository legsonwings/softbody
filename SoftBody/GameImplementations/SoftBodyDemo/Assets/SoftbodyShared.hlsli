#include "..\..\..\SharedConstants.h"

#define ROOT_SIG "CBV(b0), \
                  RootConstants(b1, num32bitconstants=2), \
                  SRV(t0), \
                  SRV(t1)"

struct Constants
{
    float3 CamPos;
    uint Padding0;
    float4x4 World;
    float4x4 WorldView;
    float4x4 WorldViewProj;
};

struct DispatchParameters
{
    uint NumTriangles;
    uint TrianglesPerInstance;
};

struct Payload
{
    uint InstanceIndices[MAX_MSGROUPS_PER_ASGROUP];
    uint StartingVertIndices[MAX_MSGROUPS_PER_ASGROUP];
    uint NumPrimitives[MAX_MSGROUPS_PER_ASGROUP];
};

struct instance_data
{
    float4 Position;
    float4 Color;
};
    
ConstantBuffer<Constants> Globals : register(b0);
ConstantBuffer<DispatchParameters> DispatchParams : register(b1);
