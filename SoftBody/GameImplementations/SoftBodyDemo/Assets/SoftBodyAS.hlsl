#include "SoftBodyShared.hlsli"

[RootSignature(ROOT_SIG)]
[NumThreads(AS_GROUP_SIZE, 1, 1)]
void main(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
{
    // Each mesh shader thread group also only deals with a single instance
    const uint num_instances = DispatchParams.NumTriangles / DispatchParams.TrianglesPerInstance;
    const uint num_groups_per_instance = ceil(float(DispatchParams.TrianglesPerInstance) / float(MAX_TRIANGLES_PER_GROUP));

    Payload payload = (Payload) 0;
    uint num_tris_processed = 0;
    for (uint instance_idx = 0; instance_idx < num_instances; ++instance_idx)
    {
        for (uint rel_group_idx = 0; rel_group_idx < num_groups_per_instance; ++rel_group_idx)
        {
            const uint groupID = instance_idx * num_groups_per_instance + rel_group_idx;
            const uint primitives_to_process = min(DispatchParams.TrianglesPerInstance - rel_group_idx * MAX_TRIANGLES_PER_GROUP, MAX_TRIANGLES_PER_GROUP);

            payload.InstanceIndices[groupID] = instance_idx;
            payload.StartingVertIndices[groupID] = num_tris_processed * 3;
            payload.NumPrimitives[groupID] = primitives_to_process;
            num_tris_processed = num_tris_processed + primitives_to_process;
        }
    }
    
    DispatchMesh(num_groups_per_instance * num_instances, 1, 1, payload);
}