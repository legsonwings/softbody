#include "Common.hlsli"

StructuredBuffer<VertexIn> triangle_vertices : register(t0);
StructuredBuffer<instance_data> instances : register(t1);

MeshShaderVertex GetVertAttribute(VertexIn vertex, uint instance_idx)
{
    MeshShaderVertex outvert;
    
    float4 const pos = float4(vertex.position, 1.f);
    outvert.instanceid = instance_idx;
    outvert.position = mul(pos, instances[instance_idx].matx).xyz;
    outvert.positionh = mul(pos, instances[instance_idx].mvpmatx);
    outvert.normal = normalize(mul(float4(vertex.normal, 0), instances[instance_idx].normalmatx).xyz);

    return outvert;
}

#define MAX_VERTICES_PER_GROUP (MAX_TRIANGLES_PER_GROUP * 3)

[RootSignature(ROOT_SIG)]
[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload payload_instanced payload,
    out indices uint3 tris[MAX_TRIANGLES_PER_GROUP],
    out vertices MeshShaderVertex verts[MAX_VERTICES_PER_GROUP]
)
{
    uint const numprims = payload.data[gid].numprims;
    SetMeshOutputCounts(numprims * 3, numprims);

    if (gtid < numprims)
    {
        // The out buffers are local to group but input buffer is global
        uint const instanceidx = (payload.data[gid].start + gtid) / dispatch_params.numprims_perinstance;
        uint const v0Idx = gtid * 3;
        uint const v1Idx = v0Idx + 1;
        uint const v2Idx = v0Idx + 2;

        tris[gtid] = uint3(v0Idx, v1Idx, v2Idx);
        uint const inVertStart = ((payload.data[gid].start + gtid) % dispatch_params.numprims_perinstance) * 3;
    
        verts[v0Idx] = GetVertAttribute(triangle_vertices[inVertStart], instanceidx);
        verts[v1Idx] = GetVertAttribute(triangle_vertices[inVertStart + 1], instanceidx);
        verts[v2Idx] = GetVertAttribute(triangle_vertices[inVertStart + 2], instanceidx);
    }
}
