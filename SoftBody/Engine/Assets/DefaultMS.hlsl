#include "Default.hlsli"

StructuredBuffer<VertexIn> TriangleVertices : register(t0);

MeshShaderVertex GetVertAttribute(VertexIn vertex)
{
    MeshShaderVertex outVert;
    
    outVert.projected_position = mul(float4(vertex.position, 1), Globals.WorldViewProj);
    outVert.color = float4(dispatch_params.color, 1.f);
    outVert.position = vertex.position;
    
    //Should this be inverse transformed?
    outVert.normal = mul(float4(vertex.normal, 0), Globals.World).xyz;
    
    return outVert;
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
    const uint numPrimitives = payload.NumPrimitives[gid];
    SetMeshOutputCounts(numPrimitives * 3, numPrimitives);

    if (gtid < numPrimitives)
    {
        // The out buffers are local to group but input buffer is global
        int v0Idx = gtid * 3;
        int v1Idx = v0Idx + 1;
        int v2Idx = v0Idx + 2;

        tris[gtid] = uint3(v0Idx, v1Idx, v2Idx);
        int inVertStart = payload.StartingVertIndices[gid] + gtid * 3;
        
        verts[v0Idx] = GetVertAttribute(TriangleVertices[inVertStart]);
        verts[v1Idx] = GetVertAttribute(TriangleVertices[inVertStart + 1]);
        verts[v2Idx] = GetVertAttribute(TriangleVertices[inVertStart + 2]);
    }
}
