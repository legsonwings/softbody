#include "common.hlsli"

StructuredBuffer<vertexin> triangle_verts : register(t0);

texturessvertex getvertattribute(vertexin vertex)
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
    int v0idx = gtid * 3;
    int v1idx = v0idx + 1;
    int v2idx = v0idx + 2;

    tris[gtid] = uint3(v0idx, v1idx, v2idx);
        
    verts[v0idx] = getvertattribute(triangle_verts[v0idx]);
    verts[v1idx] = getvertattribute(triangle_verts[v1idx]);
    verts[v2idx] = getvertattribute(triangle_verts[v2idx]);
}
