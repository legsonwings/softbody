#include "common.hlsli"

StructuredBuffer<VertexIn> triangle_verts : register(t0);

texturessvertex GetVertAttribute(VertexIn vertex)
{
    texturessvertex outvert;
    outvert.positionh = mul(float4(vertex.position, 1.f), objectconstants.mvpmatx);
    outvert.texcoord = vertex.texcoord;
    return outvert;
}

[RootSignature(ROOT_SIG)]
[NumThreads(2, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out indices uint3 tris[2],
    out vertices texturessvertex verts[6]
)
{
    const uint numprims = 2;
    SetMeshOutputCounts(numprims * 3, numprims);

    // The out buffers are local to group but input buffer is global
    int v0Idx = gtid * 3;
    int v1Idx = v0Idx + 1;
    int v2Idx = v0Idx + 2;

    tris[gtid] = uint3(v0Idx, v1Idx, v2Idx);
        
    verts[v0Idx] = GetVertAttribute(triangle_verts[v0Idx]);
    verts[v1Idx] = GetVertAttribute(triangle_verts[v1Idx]);
    verts[v2Idx] = GetVertAttribute(triangle_verts[v2Idx]);
}
