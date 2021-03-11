#include "Default.hlsli"

[RootSignature(ROOT_SIG)]
[NumThreads(AS_GROUP_SIZE, 1, 1)]
void main(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
{
    const uint num_groups = ceil(float(dispatch_params.NumTriangles) / float(MAX_TRIANGLES_PER_GROUP));

    payload_default payload = (payload_default) 0;
    uint num_tris_processed = 0;
    
    // todo : parallelize this
    for (uint group_idx = 0; group_idx < num_groups; ++group_idx)
    {
        const uint groupID = group_idx;
        const uint primitives_to_process = min(dispatch_params.NumTriangles - num_tris_processed, MAX_TRIANGLES_PER_GROUP);

        payload.StartingVertIndices[groupID] = num_tris_processed * 3;
        payload.NumPrimitives[groupID] = primitives_to_process;
        num_tris_processed = num_tris_processed + primitives_to_process;
    }
    
    DispatchMesh(num_groups, 1, 1, payload);
}