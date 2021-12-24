
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
