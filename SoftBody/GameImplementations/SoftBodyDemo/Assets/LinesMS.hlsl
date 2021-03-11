
#define ROOT_SIG "CBV(b0), \
                  RootConstants(b1, num32bitconstants=6), \
                  RootConstants(b2, num32bitconstants=1), \
                  SRV(t0), \
                  SRV(t1)"

struct Constants
{
    float3 CamPos;
    uint Padding0;
    float4x4 World;
    float4x4 WorldView;
    float4x4 WorldViewProj;
    uint NumTriangles;
    uint TrianglesPerInstance;
};

struct instance_data
{
    float3 position;
};

struct instances_information
{
    float4 color;
    uint num_instances;
    uint num_primitives_per_instance;
};

struct meshshader_info
{
    uint starting_prim_idx;
};

struct Vertex
{
    float3 position;
};

ConstantBuffer<Constants> Globals : register(b0);
ConstantBuffer<instances_information> instances_info : register(b1);
ConstantBuffer<meshshader_info> group_info : register(b2);
StructuredBuffer<Vertex> Vertices : register(t0);
StructuredBuffer<instance_data> instances : register(t1);

struct VertexOut
{
    float4 position : SV_Position;
};

#define MAX_PRIMS_SUPPORTED 64

[RootSignature(ROOT_SIG)]
[NumThreads(128, 1, 1)]
[OutputTopology("line")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out indices uint2 lines[MAX_PRIMS_SUPPORTED],
    out vertices VertexOut verts[MAX_PRIMS_SUPPORTED * 2]
)
{  
    uint const num_prims_remaining = (instances_info.num_instances * instances_info.num_primitives_per_instance) - group_info.starting_prim_idx;
    uint const num_prims = min(num_prims_remaining, MAX_PRIMS_SUPPORTED);
    uint const num_verts = num_prims * 2;

    SetMeshOutputCounts(num_verts, num_prims);

    if(gtid < num_verts)
    {
        uint const instance_idx = (group_info.starting_prim_idx + (gtid / 2)) / instances_info.num_primitives_per_instance;
        float3 const inst_pos = instances[instance_idx].position;
        uint const vert_idx = (group_info.starting_prim_idx * 2 + gtid) % (instances_info.num_primitives_per_instance * 2);
        float4 const final_pos = float4(Vertices[vert_idx].position + inst_pos, 1.f);
        float4 const proj_pos = mul(final_pos, Globals.WorldViewProj);
        verts[gtid].position = proj_pos;
    }

    if(gtid < num_prims)
    {
        uint start_idx = gtid * 2;
        lines[gtid] = uint2(start_idx, start_idx + 1);
    }
}