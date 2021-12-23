
#include "common.hlsli"

struct linevertexin
{
    float3 position;
};

StructuredBuffer<linevertexin> in_vertices : register(t0);
StructuredBuffer<instance_data> in_instances : register(t1);

struct vertexout
{
    float4 position : SV_Position;
};

// todo : remove this
//
//[RootSignature(ROOT_SIG)]
//[NumThreads(128, 1, 1)]
//[OutputTopology("line")]
//void main(
//    uint gtid : SV_GroupThreadID,
//    uint gid : SV_GroupID,
//    in payload payload_instances payload,
//    out indices uint2 lines[MAX_LINES_PER_GROUP],
//    out vertices vertexout outverts[MAX_LINES_PER_GROUP * 2]
//)
//{  
//    const uint num_prims = payload.numprims[gid];
//    SetMeshOutputCounts(num_prims * 2, num_prims);
//
//    if(gtid < num_prims)
//    {
//        int outv0Idx = gtid * 2;
//        int outv1Idx = outv0Idx + 1;
//
//        lines[gtid] = uint2(outv0Idx, outv1Idx);
//        int inputvert_start = payload.starting_vertindices[gid] + gtid * 2;
//
//        outverts[outv0Idx].position = mul(float4(in_vertices[inputvert_start].position, 1), in_instances[payload.instance_indices[gid]].mvpmatx);
//        outverts[outv1Idx].position = mul(float4(in_vertices[inputvert_start + 1].position, 1), in_instances[payload.instance_indices[gid]].mvpmatx);
//    }
//}

[RootSignature(ROOT_SIG)]
[NumThreads(128, 1, 1)]
[OutputTopology("line")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload payloaddata payload,
    out indices uint2 lines[MAX_LINES_PER_GROUP],
    out vertices vertexout verts[MAX_LINES_PER_GROUP * 2]
)
{
    uint const numprims = payload.data[gid].numprims;
    SetMeshOutputCounts(numprims * 2, numprims);

    if (gtid < numprims)
    {
        // The out buffers are local to group but input buffer is global
        uint const instanceidx = (payload.data[gid].start + gtid) / dispatch_params.numprims_perinstance;
        uint const v0idx = gtid * 3;
        uint const v1idx = v0idx + 1;

        lines[gtid] = uint2(v0idx, v1idx);
        uint const inputvert_start = ((payload.data[gid].start + gtid) % dispatch_params.numprims_perinstance) * 2;

        verts[v0idx].position = mul(float4(in_vertices[inputvert_start].position, 1), in_instances[instanceidx].mvpmatx);
        verts[v1idx].position = mul(float4(in_vertices[inputvert_start + 1].position, 1), in_instances[instanceidx].mvpmatx);
    }
}
