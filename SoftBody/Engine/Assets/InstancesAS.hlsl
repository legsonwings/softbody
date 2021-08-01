#include "Common.hlsli"

[RootSignature(ROOT_SIG)]
[NumThreads(AS_GROUP_SIZE, 1, 1)]
void main(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
{
    // Each mesh shader thread group only deals with a single instance
    const uint num_instances = dispatch_params.numprims / dispatch_params.numprims_perinstance;
    const uint num_groups_per_instance = ceil(float(dispatch_params.numprims_perinstance) / float(dispatch_params.maxprims_permsgroup));

    payload_instances payload = (payload_instances) 0;
    uint num_prims_processed = 0;
    
    // todo : parallelize this
    for (uint instance_idx = 0; instance_idx < num_instances; ++instance_idx)
    {
        for (uint rel_group_idx = 0; rel_group_idx < num_groups_per_instance; ++rel_group_idx)
        {
            uint curr_groupid = instance_idx * num_groups_per_instance + rel_group_idx;
            uint primitives_to_process = min(dispatch_params.numprims_perinstance - rel_group_idx * dispatch_params.maxprims_permsgroup, dispatch_params.maxprims_permsgroup);

            payload.instance_indices[curr_groupid] = instance_idx;
            payload.starting_vertindices[curr_groupid] = (num_prims_processed * dispatch_params.numverts_perprim % dispatch_params.numprims_perinstance);
            payload.numprims[curr_groupid] = primitives_to_process;
            num_prims_processed += primitives_to_process;
        }

        payload.instance_indices[0] = 0;
    }
    
    DispatchMesh(num_groups_per_instance * num_instances, 1, 1, payload);
}