#include "common.hlsli"

StructuredBuffer<VertexIn> triangle_verts : register(t0);

MeshShaderVertex GetVertAttribute(VertexIn vertex)
{
    MeshShaderVertex outvert;
    
    float4 const pos = float4(vertex.position, 1.f);
    outvert.position = mul(pos, objectconstants.matx).xyz;
    outvert.positionh = mul(pos, objectconstants.mvpmatx);
    outvert.normal = normalize(mul(float4(vertex.normal, 0), objectconstants.normalmatx).xyz);
    
    return outvert;
}

#define MAX_VERTICES_PER_GROUP (MAX_TRIANGLES_PER_GROUP * 3)

[RootSignature(ROOT_SIG)]
[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload payload_default payload,
    out indices uint3 tris[MAX_TRIANGLES_PER_GROUP],
    out vertices MeshShaderVertex verts[MAX_VERTICES_PER_GROUP]
)
{
    const uint numprims = payload.numprims[gid];
    SetMeshOutputCounts(numprims * 3, numprims);

    if (gtid < numprims)
    {
        // The out buffers are local to group but input buffer is global
        int v0Idx = gtid * 3;
        int v1Idx = v0Idx + 1;
        int v2Idx = v0Idx + 2;

        tris[gtid] = uint3(v0Idx, v1Idx, v2Idx);
        int in_vertstart = payload.startingvert_indices[gid] + gtid * 3;
        
        verts[v0Idx] = GetVertAttribute(triangle_verts[in_vertstart]);
        verts[v1Idx] = GetVertAttribute(triangle_verts[in_vertstart + 1]);
        verts[v2Idx] = GetVertAttribute(triangle_verts[in_vertstart + 2]);
    }
}
