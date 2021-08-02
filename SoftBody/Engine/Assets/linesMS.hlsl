#include "Common.hlsli"

struct vertexin
{
    float3 position;
};

StructuredBuffer<vertexin> in_vertices : register(t0);

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
    in payload payload_default payload,
    out indices uint2 lines[MAX_LINES_PER_GROUP],
    out vertices vertexout verts[MAX_LINES_PER_GROUP * 2]
)
{
    uint const num_prims = payload.numprims[gid];
    SetMeshOutputCounts(num_prims * 2, num_prims);

    if (gtid < num_prims)
    {
        uint const outv0Idx = gtid * 2;
        uint const outv1Idx = outv0Idx + 1;

        lines[gtid] = uint2(outv0Idx, outv1Idx);
        uint const inputvert_start = payload.startingvert_indices[gid] + gtid * 2;

        verts[outv0Idx].position = mul(mul(float4(in_vertices[inputvert_start].position, 1), objectconstants.matx), Globals.viewproj);
        verts[outv1Idx].position = mul(mul(float4(in_vertices[inputvert_start + 1].position, 1), objectconstants.matx), Globals.viewproj);
    }
}
