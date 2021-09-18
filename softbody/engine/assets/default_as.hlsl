#include "common.hlsli"

[RootSignature(ROOT_SIG)]
[NumThreads(AS_GROUP_SIZE, 1, 1)]
void main(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
{
    const uint num_groups = ceil(float(dispatch_params.numprims) / float(dispatch_params.maxprims_permsgroup));

    payload_default payload = (payload_default) 0;
    uint numprims_processed = 0;
    
    // todo : parallelize this
    for (uint group_idx = 0; group_idx < num_groups; ++group_idx)
    {
        const uint groupID = group_idx;
        const uint primitives_to_process = min(dispatch_params.numprims - numprims_processed, dispatch_params.maxprims_permsgroup);

        payload.startingvert_indices[groupID] = numprims_processed * dispatch_params.numverts_perprim;
        payload.numprims[groupID] = primitives_to_process;
        numprims_processed += primitives_to_process;
    }
    
    DispatchMesh(num_groups, 1, 1, payload);
}