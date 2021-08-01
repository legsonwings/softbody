
#include "../../../Engine/Assets/Common.hlsli"

struct vertexin
{
    float3 position;
};

StructuredBuffer<vertexin> vertices : register(t0);
StructuredBuffer<instance_data> instances : register(t1);

struct vertexout
{
    float4 position : SV_Position;
};

[RootSignature(ROOT_SIG)]
[NumThreads(128, 1, 1)]
[OutputTopology("line")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload payload_instances payload,
    out indices uint2 lines[MAX_LINES_PER_GROUP],
    out vertices vertexout verts[MAX_LINES_PER_GROUP * 2]
)
{  
    const uint num_prims = payload.numprims[gid];
    SetMeshOutputCounts(num_prims * 2, num_prims);

    if(gtid < num_prims)
    {
        int const outv0Idx = gtid * 2;
        int const outv1Idx = outv0Idx + 1;

        lines[gtid] = uint2(outv0Idx, outv1Idx);
        int const inputvert_start = payload.starting_vertindices[gid] + gtid * 2;

        float4x4 instmat = instances[payload.instance_indices[gid]].mat;
        float4x4 proj = Globals.viewproj;
        float4 v0 = mul(float4(vertices[inputvert_start].position, 1), instmat);
        float4 v1 = mul(float4(vertices[inputvert_start + 1].position, 1), instmat);
        float4 v0f = mul(float4(vertices[inputvert_start].position, 1), proj);
        float4 v1f = mul(float4(vertices[inputvert_start + 1].position, 1), proj);

        verts[outv0Idx].position = mul(mul(float4(vertices[inputvert_start].position, 1), instances[payload.instance_indices[gid]].mat), Globals.viewproj);
        verts[outv1Idx].position = mul(mul(float4(vertices[inputvert_start + 1].position, 1), instances[payload.instance_indices[gid]].mat), Globals.viewproj);
    }
}