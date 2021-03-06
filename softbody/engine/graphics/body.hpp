#include "body.h"
#include "globalresources.h"
#include "engine/sharedconstants.h"

#include <functional>

namespace gfx
{
    void dispatch(resource_bindings const &bindings, bool wireframe = false, bool twosided = false, uint dispatchx = 1);

    template<sbody_c body_t, topology prim_t>
    inline body_static<body_t, prim_t>::body_static(rawbody_t _body, bodyparams const& _params) : bodyinterface(_params), body(std::move(_body))
    {
        get_vertices = [](body_t const& geom) { return geom.vertices(); };
        get_instancedata = [](body_t const& geom) { return geom.instancedata(); };
    }

    template<sbody_c body_t, topology prim_t>
    inline body_static<body_t, prim_t>::body_static(body_t const& _body, vertexfetch_r (rawbody_t::* vfun)() const, instancedatafetch_r (rawbody_t::* ifun)() const, bodyparams const& _params) : bodyinterface(_params), body(_body)
    {
        get_vertices = [vfun](body_t const& geom) { return std::invoke(vfun, geom); };
        get_instancedata = [ifun](body_t const& geom) { return std::invoke(ifun, geom); };
    }

    template<sbody_c body_t, topology prim_t>
    std::vector<ComPtr<ID3D12Resource>> body_static<body_t, prim_t>::create_resources()
    {
        auto const vbupload = _vertexbuffer.createresources(get_vertices(body));
        _instancebuffer.createresource(get_instancedata(body));

        assert(_vertexbuffer.count() < ASGROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * topologyconstants<prim_t>::maxprims_permsgroup * topologyconstants<prim_t>::numverts_perprim);

        // return the upload buffer so that engine can keep it alive until vertex data has been uploaded to gpu
        return { vbupload };
    }

    struct dispatchparams
    {
        uint32_t numprims;
        uint32_t numverts_perprim;
        uint32_t maxprims_permsgroup;
        uint32_t numprims_perinstance;
    };

    template<sbody_c body_t, topology prim_t>
    inline void body_static<body_t, prim_t>::render(float dt, renderparams const &params)
    {
        auto const foundpso = globalresources::get().psomap().find(getparams().psoname);
        if (foundpso == globalresources::get().psomap().cend())
        {
            assert("pso not found");
            return;
        }

        _instancebuffer.updateresource(get_instancedata(body));

        dispatchparams dispatch_params;
        dispatch_params.numverts_perprim = topologyconstants<prim_t>::numverts_perprim;
        dispatch_params.numprims_perinstance = static_cast<uint32_t>(_vertexbuffer.count() / topologyconstants<prim_t>::numverts_perprim);
        dispatch_params.numprims = static_cast<uint32_t>(_instancebuffer.count() * dispatch_params.numprims_perinstance);
        dispatch_params.maxprims_permsgroup = topologyconstants<prim_t>::maxprims_permsgroup;

        resource_bindings bindings;
        bindings.constant = { 0, globalresources::get().cbuffer().gpuaddress() };
        bindings.vertex = { 3, _vertexbuffer.gpuaddress() };
        bindings.instance = { 4, _instancebuffer.gpuaddress() };
        bindings.pipelineobjs = foundpso->second;
        bindings.rootconstants.slot = 2;
        bindings.rootconstants.values.resize(sizeof(dispatch_params));

        uint const numasthreads = static_cast<uint>(std::ceil(static_cast<float>(dispatch_params.numprims) / static_cast<float>(ASGROUP_SIZE * dispatch_params.maxprims_permsgroup)));
        assert(numasthreads < 128);
        memcpy(bindings.rootconstants.values.data(), &dispatch_params, sizeof(dispatch_params));
        dispatch(bindings, params.wireframe, gfx::globalresources::get().mat(getparams().matname).ex(), numasthreads);
    }

    template<dbody_c body_t, topology prim_t>
    inline body_dynamic<body_t, prim_t>::body_dynamic(rawbody_t _body, bodyparams const& _params, stdx::vecui2 texdims)  : bodyinterface(_params), body(std::move(_body))
    {
        _texture._dims = texdims;
        get_vertices = [](body_t const& geom) { return geom.vertices(); };
    }

    template<dbody_c body_t, topology prim_t>
    inline body_dynamic<body_t, prim_t>::body_dynamic(body_t const& _body, vertexfetch_r (rawbody_t::*fun)() const, bodyparams const& _params, stdx::vecui2 texdims) : bodyinterface(_params), body(_body)
    {
        _texture._dims = texdims;
        get_vertices = [fun](body_t const& geom) { return std::invoke(fun, geom); };
    }

    template<dbody_c body_t, topology prim_t>
    inline std::vector<ComPtr<ID3D12Resource>> body_dynamic<body_t, prim_t>::create_resources()
    {
        _cbuffer.createresource();
        auto const& verts = get_vertices(body);
        _vertexbuffer.createresource(verts);
        _texture.createresource(0, getparams().dims, body.texturedata(), gfx::globalresources::get().srvheap().Get());

        assert(_vertexbuffer.count() < ASGROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * topologyconstants<prim_t>::maxprims_permsgroup * topologyconstants<prim_t>::numverts_perprim);
        return {};
    }

    template<dbody_c body_t, topology prim_t>
    inline void body_dynamic<body_t, prim_t>::update(float dt) 
    { 
        // update only if we own this body
        if constexpr(std::is_same_v<body_t, rawbody_t>) body.update(dt);
    }

    template<dbody_c body_t, topology prim_t>
    inline void body_dynamic<body_t, prim_t>::render(float dt, renderparams const &params)
    {
        auto const foundpso = globalresources::get().psomap().find(getparams().psoname);
        if (foundpso == globalresources::get().psomap().cend())
        {
            assert("pso not found");
            return;
        }

        _vertexbuffer.updateresource(get_vertices(body));
        _texture.updateresource(body.texturedata());
        _cbuffer.updateresource(objectconstants{ matrix::CreateTranslation(body.center()), globalresources::get().view(), globalresources::get().mat(getparams().matname) });
        assert(_vertexbuffer.count() < ASGROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * topologyconstants<prim_t>::maxprims_permsgroup * topologyconstants<prim_t>::numverts_perprim);
        
        dispatchparams dispatch_params;
        dispatch_params.numverts_perprim = topologyconstants<prim_t>::numverts_perprim;
        dispatch_params.numprims = static_cast<uint32_t>(_vertexbuffer.count() / topologyconstants<prim_t>::numverts_perprim);
        dispatch_params.maxprims_permsgroup = topologyconstants<prim_t>::maxprims_permsgroup;

        resource_bindings bindings;
        bindings.constant = { 0, globalresources::get().cbuffer().gpuaddress() };
        bindings.objectconstant = { 1, _cbuffer.gpuaddress() };
        bindings.vertex = { 3, _vertexbuffer.gpuaddress() };
        bindings.pipelineobjs = foundpso->second;
        bindings.rootconstants.slot = 2;
        bindings.rootconstants.values.resize(sizeof(dispatch_params));

        if(_texture.size() > 0)
            bindings.texture = { 5, _texture.deschandle() };

        memcpy(bindings.rootconstants.values.data(), &dispatch_params, sizeof(dispatch_params));
        dispatch(bindings, params.wireframe, gfx::globalresources::get().mat(getparams().matname).ex());
    }
}