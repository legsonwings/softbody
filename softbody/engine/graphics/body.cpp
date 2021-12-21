#include "body.h"
#include "globalresources.h"

#include "d3dx12.h"

void gfx::dispatch(resource_bindings const& bindings, bool wireframe, bool twosided, uint dispatchx)
{
    auto cmd_list = gfx::globalresources::get().cmdlist();

    cmd_list->SetGraphicsRootSignature(bindings.pipelineobjs.root_signature.Get());
    cmd_list->SetDescriptorHeaps(1, gfx::globalresources::get().srvheap().GetAddressOf());
    cmd_list->SetGraphicsRootConstantBufferView(bindings.constant.slot, bindings.constant.address);
    
    if (bindings.objectconstant.address != 0)
        cmd_list->SetGraphicsRootConstantBufferView(bindings.objectconstant.slot, bindings.objectconstant.address);
    cmd_list->SetGraphicsRootShaderResourceView(bindings.vertex.slot, bindings.vertex.address);
    cmd_list->SetGraphicsRoot32BitConstants(bindings.rootconstants.slot, static_cast<UINT>(bindings.rootconstants.values.size() / 4), bindings.rootconstants.values.data(), 0);

    if (bindings.instance.address != 0)
        cmd_list->SetGraphicsRootShaderResourceView(bindings.instance.slot, bindings.instance.address);

    if(bindings.texture.deschandle.ptr)
        cmd_list->SetGraphicsRootDescriptorTable(bindings.texture.slot, bindings.texture.deschandle);

    if (wireframe && bindings.pipelineobjs.pso_wireframe)
        cmd_list->SetPipelineState(bindings.pipelineobjs.pso_wireframe.Get());
    else if (twosided && bindings.pipelineobjs.pso_twosided)
        cmd_list->SetPipelineState(bindings.pipelineobjs.pso_twosided.Get());
    else
        cmd_list->SetPipelineState(bindings.pipelineobjs.pso.Get());

    cmd_list->DispatchMesh(static_cast<UINT>(dispatchx), 1, 1);
}