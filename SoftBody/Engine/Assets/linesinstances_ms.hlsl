
#include "Common.hlsli"

struct vertexin
{
    float3 position;
};

StructuredBuffer<vertexin> in_vertices : register(t0);
StructuredBuffer<instance_data> in_instances : register(t1);

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
    out vertices vertexout outverts[MAX_LINES_PER_GROUP * 2]
)
{  
    const uint num_prims = payload.numprims[gid];
    SetMeshOutputCounts(num_prims * 2, num_prims);

    if(gtid < num_prims)
    {
        int outv0Idx = gtid * 2;
        int outv1Idx = outv0Idx + 1;

        lines[gtid] = uint2(outv0Idx, outv1Idx);
        int inputvert_start = payload.starting_vertindices[gid] + gtid * 2;

        outverts[outv0Idx].position = mul(float4(in_vertices[inputvert_start].position, 1), in_instances[payload.instance_indices[gid]].mvpmatx);
        outverts[outv1Idx].position = mul(float4(in_vertices[inputvert_start + 1].position, 1), in_instances[payload.instance_indices[gid]].mvpmatx);
    }
}