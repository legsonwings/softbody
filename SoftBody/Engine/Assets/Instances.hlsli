#include "Common.hlsli"

#define ROOT_SIG "CBV(b0), \
                  RootConstants(b1, num32bitconstants=2), \
                  SRV(t0), \
                  SRV(t1)"

struct dispatch_parameters
{
    uint NumTriangles;
    uint TrianglesPerInstance;
};

struct instance_data
{
    float3 Position;
    float3 Color;
};

struct payload_instances
{
    uint InstanceIndices[MAX_MSGROUPS_PER_ASGROUP];
    uint StartingVertIndices[MAX_MSGROUPS_PER_ASGROUP];
    uint NumPrimitives[MAX_MSGROUPS_PER_ASGROUP];
};

ConstantBuffer<dispatch_parameters> dispatch_params : register(b1);
