#include "SoftBodyShared.hlsli"

struct VertexIn
{
    float3 Position;
    float3 Normal;
};

struct VertexOut
{
    float4 PositionHS : SV_Position;
    // TODO : Move this to primitive attribute instead or pass instance id to ps
    float4 Color : COLOR0;
    float3 Position : POSITION0;
    float3 Normal : NORMAL0;
};

StructuredBuffer<instance_data> Instances : register(t1);
StructuredBuffer<VertexIn> TriangleVertices : register(t0);

VertexOut GetVertAttribute(VertexIn vertex, uint instance_idx)
{
    VertexOut outVert;
    
    outVert.PositionHS = mul(float4(vertex.Position + (float3)Instances[instance_idx].Position, 1), Globals.WorldViewProj);
    outVert.Color = Instances[instance_idx].Color;
    outVert.Position = vertex.Position;
    outVert.Normal = mul(float4(vertex.Normal, 0), Globals.World).xyz;
    
    return outVert;
}

#define MAX_VERTICES_PER_GROUP (MAX_TRIANGLES_PER_GROUP * 3)

[RootSignature(ROOT_SIG)]
[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload Payload payload,
    out indices uint3 tris[MAX_TRIANGLES_PER_GROUP],
    out vertices VertexOut verts[MAX_VERTICES_PER_GROUP]
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
        
        verts[v0Idx] = GetVertAttribute(TriangleVertices[inVertStart], payload.InstanceIndices[gid]);
        verts[v1Idx] = GetVertAttribute(TriangleVertices[inVertStart + 1], payload.InstanceIndices[gid]);
        verts[v2Idx] = GetVertAttribute(TriangleVertices[inVertStart + 2], payload.InstanceIndices[gid]);

        //verts[v0Idx].PositionHS = float4(-0.5f, 0.f, 0.f, 1.f);
        //verts[v1Idx].PositionHS = float4(0.f, 1.f, 0.f, 1.f);
        //verts[v2Idx].PositionHS = float4(0.5f, 0.f, 0.f, 1.f);
    }
}
