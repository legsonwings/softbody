#include "common.hlsli"

[RootSignature(ROOT_SIG)]
[NumThreads(ASGROUP_SIZE, 1, 1)]
void main(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
{
    uint const maxprimsperasg = dispatch_params.maxprims_permsgroup * ASGROUP_SIZE;
    uint const nummyprims = min(dispatch_params.numprims - gid * maxprimsperasg, maxprimsperasg);
    uint const nummsgroups = ceil(float(nummyprims) / float(dispatch_params.maxprims_permsgroup));

    payloaddata payload = (payloaddata)0;
    if (gtid < nummsgroups)
    {
        payload.data[gtid].start = gid * maxprimsperasg + gtid * dispatch_params.maxprims_permsgroup;
        payload.data[gtid].numprims = min(nummyprims - payload.data[gtid].start, dispatch_params.maxprims_permsgroup);
    }

    DispatchMesh(nummsgroups, 1, 1, payload);
}

//// todo : remove this
//
//[RootSignature(ROOT_SIG)]
//[NumThreads(ASGROUP_SIZE, 1, 1)]
//void main(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
//{
//    const uint num_groups = ceil(float(dispatch_params.numprims) / float(dispatch_params.maxprims_permsgroup));
//
//    payloaddata payload = (payloaddata) 0;
//    uint numprims_processed = 0;
//    
//    // todo : parallelize this
//    for (uint group_idx = 0; group_idx < num_groups; ++group_idx)
//    {
//        const uint groupID = group_idx;
//        const uint primitives_to_process = min(dispatch_params.numprims - numprims_processed, dispatch_params.maxprims_permsgroup);
//
//        payload.data[groupID].start = numprims_processed * dispatch_params.numverts_perprim;
//        payload.data[groupID].numprims = primitives_to_process;
//        numprims_processed += primitives_to_process;
//    }
//    
//    DispatchMesh(num_groups, 1, 1, payload);
//}