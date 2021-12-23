#include "common.hlsli"

StructuredBuffer<vertexin> triangle_vertices : register(t0);
StructuredBuffer<instance_data> instances : register(t1);

meshshadervertex getvertattribute(vertexin vertex, uint instance_idx)
{
    meshshadervertex outvert;
    
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
    in payload payloaddata payload,
    out indices uint3 tris[MAX_TRIANGLES_PER_GROUP],
    out vertices meshshadervertex verts[MAX_VERTICES_PER_GROUP]
)
{
    uint const numprims = payload.data[gid].numprims;
    SetMeshOutputCounts(numprims * 3, numprims);

    if (gtid < numprims)
    {
        // The out buffers are local to group but input buffer is global
        uint const instanceidx = (payload.data[gid].start + gtid) / dispatch_params.numprims_perinstance;
        uint const v0idx = gtid * 3;
        uint const v1idx = v0idx + 1;
        uint const v2idx = v0idx + 2;

        tris[gtid] = uint3(v0idx, v1idx, v2idx);
        uint const invertstart = ((payload.data[gid].start + gtid) % dispatch_params.numprims_perinstance) * 3;
    
        verts[v0idx] = getvertattribute(triangle_vertices[invertstart], instanceidx);
        verts[v1idx] = getvertattribute(triangle_vertices[invertstart + 1], instanceidx);
        verts[v2idx] = getvertattribute(triangle_vertices[invertstart + 2], instanceidx);
    }
}
