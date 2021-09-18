#pragma once
#include "..\sharedconstants.h"

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

struct VertexIn
{
    float3 position;
    float3 normal;
};

struct MeshShaderVertex
{
    uint instanceid : instance_id;
    float4 positionh : SV_Position;
    float3 position : POSITION0;
    float3 normal : NORMAL0;
};

struct light
{
    float3 color;
    float range;
    float3 position;
    float padding1;
    float3 direction;
    float padding2;
};

struct material
{
    float3 fresnelr;
    float roughness;
    float4 diffusealbedo;
};

struct sceneconstants
{
    float3 campos;
    uint padding0;
    float4 ambient;
    light lights[NUM_LIGHTS];
};

struct instance_data
{
    float4x4 matx;
    float4x4 normalmatx;
    float4x4 mvpmatx;
    material mat;
};

struct object_constants
{
    float4x4 matx;
    float4x4 normalmatx;
    float4x4 mvpmatx;
    material mat;
};

ConstantBuffer<sceneconstants> globals : register(b0);
ConstantBuffer<object_constants> objectconstants : register(b1);
ConstantBuffer<dispatch_parameters> dispatch_params : register(b2);