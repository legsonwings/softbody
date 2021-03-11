#include "Common.hlsli"

#define ROOT_SIG "CBV(b0), \
                  RootConstants(b1, num32bitconstants=4), \
                  SRV(t0), \
                  SRV(t1)"

struct dispatch_parameters
{
    uint NumTriangles;
    float3 color;
};

struct payload_default
{
    uint StartingVertIndices[MAX_MSGROUPS_PER_ASGROUP];
    uint NumPrimitives[MAX_MSGROUPS_PER_ASGROUP];
};

ConstantBuffer<dispatch_parameters> dispatch_params : register(b1);