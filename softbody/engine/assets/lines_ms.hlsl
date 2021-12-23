#include "common.hlsli"

struct linevertexin
{
    float3 position;
};

StructuredBuffer<linevertexin> in_vertices : register(t0);

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
    uint const num_prims = payload.data[gid].numprims;
    SetMeshOutputCounts(num_prims * 2, num_prims);

    if (gtid < num_prims)
    {
        uint const outv0idx = gtid * 2;
        uint const outv1idx = outv0idx + 1;

        lines[gtid] = uint2(outv0idx, outv1idx);
        uint const inputvert_start = payload.data[gid].start + gtid * 2;

        verts[outv0idx].position = mul(float4(in_vertices[inputvert_start].position, 1), objectconstants.mvpmatx);
        verts[outv1idx].position = mul(float4(in_vertices[inputvert_start + 1].position, 1), objectconstants.mvpmatx);
    }
}
