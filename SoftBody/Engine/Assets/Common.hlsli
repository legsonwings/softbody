#include "..\..\SharedConstants.h"

#define ROOT_SIG "CBV(b0), \
                  CBV(b1), \
                  RootConstants(b2, num32bitconstants=7), \
                  SRV(t0), \
                  SRV(t1)"

// todo : these should be array of structs for ease of debugging and efficiency
struct payload_default
{
    uint startingvert_indices[MAX_MSGROUPS_PER_ASGROUP];
    uint numprims[MAX_MSGROUPS_PER_ASGROUP];
};

struct payload_instances
{
    uint instance_indices[MAX_MSGROUPS_PER_ASGROUP];
    uint starting_vertindices[MAX_MSGROUPS_PER_ASGROUP];
    uint numprims[MAX_MSGROUPS_PER_ASGROUP];
};

struct dispatch_parameters
{
    uint numprims;
    uint numverts_perprim;
    uint maxprims_permsgroup;
    uint numprims_perinstance;
};

struct material
{
    float3 fresnelr;
    float roughness;
    float4 diffusealbedo;
};

struct Constants
{
    float3 campos;
    uint padding0;
    float4x4 view;
    float4x4 viewproj;
};

struct VertexIn
{
    float3 position;
    float3 normal;
};

struct instance_data
{
    float4x4 matx;
    float4x4 invmatx;
    material mat;
};

struct object_constants
{
    float4x4 matx;
    float4x4 invmatx;
    material mat;
};

struct MeshShaderVertex
{
    float4 projected_position : SV_Position;
    // todo : Move this to primitive attribute instead or pass instance id to ps
    float4 color : COLOR0;
    float3 position : POSITION0;
    float3 normal : NORMAL0;
};

ConstantBuffer<Constants> Globals : register(b0);
ConstantBuffer<object_constants> objectconstants : register(b1);
ConstantBuffer<dispatch_parameters> dispatch_params : register(b2);