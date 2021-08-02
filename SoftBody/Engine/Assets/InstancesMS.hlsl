#include "Common.hlsli"

StructuredBuffer<VertexIn> triangle_vertices : register(t0);
StructuredBuffer<instance_data> instances : register(t1);

MeshShaderVertex GetVertAttribute(VertexIn vertex, uint instance_idx)
{
    MeshShaderVertex outvert;
    
    outvert.instanceid = instance_idx;
    outvert.positionh = mul(mul(float4(vertex.position, 1), instances[instance_idx].matx), Globals.viewproj);
    outvert.position = vertex.position;
    outvert.normal = mul(float4(vertex.normal, 0), instances[instance_idx].invmatx).xyz;
    
    return outvert;
}

#define MAX_VERTICES_PER_GROUP (MAX_TRIANGLES_PER_GROUP * 3)

[RootSignature(ROOT_SIG)]
[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload payload_instances payload,
    out indices uint3 tris[MAX_TRIANGLES_PER_GROUP],
    out vertices MeshShaderVertex verts[MAX_VERTICES_PER_GROUP]
)
{
    const uint num_prims = payload.numprims[gid];
    SetMeshOutputCounts(num_prims * 3, num_prims);

    if (gtid < num_prims)
    {
        // The out buffers are local to group but input buffer is global
        int v0Idx = gtid * 3;
        int v1Idx = v0Idx + 1;
        int v2Idx = v0Idx + 2;

        tris[gtid] = uint3(v0Idx, v1Idx, v2Idx);
        int inVertStart = payload.starting_vertindices[gid] + gtid * 3;
        
        verts[v0Idx] = GetVertAttribute(triangle_vertices[inVertStart], payload.instance_indices[gid]);
        verts[v1Idx] = GetVertAttribute(triangle_vertices[inVertStart + 1], payload.instance_indices[gid]);
        verts[v2Idx] = GetVertAttribute(triangle_vertices[inVertStart + 2], payload.instance_indices[gid]);
    }
}
