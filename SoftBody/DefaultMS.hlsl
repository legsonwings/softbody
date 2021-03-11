
#define ROOT_SIG ""

struct VertexOut
{
    float4 pos : SV_Position;
};

[RootSignature(ROOT_SIG)]
[NumThreads(1, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out indices uint3 tris[1],
    out vertices VertexOut verts[3]
)
{
   SetMeshOutputCounts(3, 1);

    if (gtid < 1)
    {
        tris[0] = uint3(0, 1, 2);
        verts[0].pos = float4(-0.5f, 0.f, 0.f, 1.f);
        verts[1].pos = float4(0.f, 0.5f, 0.f, 1.f);
        verts[2].pos = float4(0.5f, 0.f, 0.f, 1.f);
    }
}