#include "common.hlsli"

[RootSignature(ROOT_SIG)]
[NumThreads(INSTANCE_ASGROUP_SIZE, 1, 1)]
void main(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
{
    uint const maxprimsperasg = dispatch_params.maxprims_permsgroup * INSTANCE_ASGROUP_SIZE;
    uint const nummyprims = min(dispatch_params.numprims - gid * maxprimsperasg, maxprimsperasg);
    uint const nummsgroups = ceil(float(nummyprims) / float(dispatch_params.maxprims_permsgroup));

    payload_instanced payload = (payload_instanced)0;
    if (gtid < nummsgroups)
    {
        payload.data[gtid].start = gid * maxprimsperasg + gtid * dispatch_params.maxprims_permsgroup;
        payload.data[gtid].numprims = min(nummyprims - payload.data[gtid].start, dispatch_params.maxprims_permsgroup);
    }

    DispatchMesh(nummsgroups, 1, 1, payload);
}