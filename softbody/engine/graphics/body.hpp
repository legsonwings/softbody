#include "body.h"
#include "gfxutils.h"
#include "engine/sharedconstants.h"

#include <string>
#include <functional>

namespace gfx
{
    using default_and_upload_buffers = std::pair<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>>;

    void dispatch(resource_bindings const &bindings, bool wireframe = false, bool twosided = false);
    default_and_upload_buffers create_vertexbuffer_default(void* vertexdata_start, std::size_t const vb_size);
    ComPtr<ID3D12Resource> createupdate_uploadbuffer(uint8_t** mapped_buffer, void* data_start, std::size_t const perframe_buffersize);
    D3D12_GPU_VIRTUAL_ADDRESS get_perframe_gpuaddress(D3D12_GPU_VIRTUAL_ADDRESS start, UINT64 perframe_buffersize);
    void update_perframebuffer(uint8_t* mapped_buffer, void* data_start, std::size_t const perframe_buffersize);
    
    template<typename body_t, topology primitive_t>
    inline body_static<body_t, primitive_t>::body_static(rawbody_t _body, bodyparams const& _params) : bodyinterface(_params), body(std::move(_body))
    {
        get_vertices = [](body_t const& geom) { return geom.gvertices(); };
        get_instancedata = [](body_t const& geom) { return geom.instancedata(); };
    }

    template<typename body_t, topology primitive_t>
    inline body_static<body_t, primitive_t>::body_static(body_t const& _body, vertexfetch_r (rawbody_t::* vfun)() const, instancedatafetch_r (rawbody_t::* ifun)() const, bodyparams const& _params) : bodyinterface(_params), body(_body)
    {
        get_vertices = [vfun](body_t const& geom) { return std::invoke(vfun, geom); };
        get_instancedata = [ifun](body_t const& geom) { return std::invoke(ifun, geom); };
    }

    template<typename body_t, topology primitive_t>
    std::vector<ComPtr<ID3D12Resource>> body_static<body_t, primitive_t>::create_resources()
    {
        m_vertices = get_vertices(body);
        m_instancedata = get_instancedata(body);
        num_instances = m_instancedata.size();

        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);

        auto const vb_resources = create_vertexbuffer_default(m_vertices.data(), get_vertexbuffersize());
        m_vertexbuffer = vb_resources.first;

        m_instance_buffer = createupdate_uploadbuffer(&m_instancebuffer_mapped, m_instancedata.data(), get_instancebuffersize());

        // return the upload buffer so that engine can keep it alive until vertex data has been uploaded to gpu
        return { vb_resources.second };
    }

    template<typename body_t, topology primitive_t>
    inline void body_static<body_t, primitive_t>::render(float dt, renderparams const &params)
    {
        auto const foundpso = gfx::getpsomap().find(getparams().psoname);
        if (foundpso == gfx::getpsomap().cend())
        {
            assert("pso not found");
            return;
        }

        update_instancebuffer();

        struct
        {
            uint32_t numprims;
            uint32_t numverts_perprim;
            uint32_t maxprims_permsgroup;
            uint32_t numprims_perinstance;
        } dispatch_params;

        dispatch_params.numverts_perprim = topologyconstants<primitive_t>::numverts_perprim;
        dispatch_params.numprims_perinstance = static_cast<uint32_t>(get_numvertices() / topologyconstants<primitive_t>::numverts_perprim);
        dispatch_params.numprims = static_cast<uint32_t>(get_numinstances() * dispatch_params.numprims_perinstance);
        dispatch_params.maxprims_permsgroup = topologyconstants<primitive_t>::maxprims_permsgroup;

        resource_bindings bindings;
        bindings.constant = { 0, params.cbaddress };
        bindings.vertex = { 3, get_vertexbuffer_gpuaddress() };
        bindings.instance = { 4, get_instancebuffer_gpuaddress() };
        bindings.pipelineobjs = foundpso->second;
        bindings.rootconstants.slot = 2;
        bindings.rootconstants.values.resize(sizeof(dispatch_params));

        memcpy(bindings.rootconstants.values.data(), &dispatch_params, sizeof(dispatch_params));
        dispatch(bindings, params.wireframe, gfx::getmat(getparams().matname).ex());
    }

    template<typename body_t, topology primitive_t>
    inline const geometry::aabb body_static<body_t, primitive_t>::getaabb() const { return body.gaabb(); }

    template<typename body_t, topology primitive_t>
    inline D3D12_GPU_VIRTUAL_ADDRESS body_static<body_t, primitive_t>::get_instancebuffer_gpuaddress() const
    {
        return get_perframe_gpuaddress(m_instance_buffer->GetGPUVirtualAddress(), get_instancebuffersize());
    }

    template<typename body_t, topology primitive_t>
    inline void body_static<body_t, primitive_t>::update_instancebuffer()
    {
        m_instancedata = get_instancedata(body);
        update_perframebuffer(m_instancebuffer_mapped, m_instancedata.data(), get_instancebuffersize());
    }

    template<typename body_t, topology primitive_t>
    inline body_dynamic<body_t, primitive_t>::body_dynamic(rawbody_t _body, bodyparams const& _params)  : bodyinterface(_params), body(std::move(_body))
    {
        get_vertices = [](body_t const& geom) { return geom.gvertices(); };
    }

    template<typename body_t, topology primitive_t>
    inline body_dynamic<body_t, primitive_t>::body_dynamic(body_t const& _body, vertexfetch_r (rawbody_t::*fun)() const, bodyparams const& _params) : bodyinterface(_params), body(_body)
    {
        get_vertices = [fun](body_t const& geom) { return std::invoke(fun, geom); };
    }

    template<typename body_t, topology primitive_t>
    inline std::vector<ComPtr<ID3D12Resource>> body_dynamic<body_t, primitive_t>::create_resources()
    {
        m_vertices = get_vertices(body);
        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);

        cbuffer.createresources<objectconstants>();
        m_vertexbuffer = createupdate_uploadbuffer(&m_vertexbuffer_databegin, m_vertices.data(), get_vertexbuffersize());
        return std::vector<ComPtr<ID3D12Resource>>();
    }

    template<typename body_t, topology primitive_t>
    inline void body_dynamic<body_t, primitive_t>::update(float dt) 
    { 
        // update only if we own this body
        if constexpr(std::is_same_v<body_t, rawbody_t>) body.update(dt);
    }

    template<typename body_t, topology primitive_t>
    inline void body_dynamic<body_t, primitive_t>::render(float dt, renderparams const &params)
    {
        m_vertices = get_vertices(body);
        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);

        auto const foundpso = gfx::getpsomap().find(getparams().psoname);
        if (foundpso == gfx::getpsomap().cend())
        {
            assert("pso not found");
            return;
        }

        update_constbuffer();
        update_vertexbuffer();
        
        struct
        {
            uint32_t numprims;
            uint32_t numverts_perprim;
            uint32_t maxprims_permsgroup;
            uint32_t numprims_perinstance;
        } dispatch_params;

        dispatch_params.numverts_perprim = topologyconstants<primitive_t>::numverts_perprim;
        dispatch_params.numprims = static_cast<uint32_t>(get_numvertices() / topologyconstants<primitive_t>::numverts_perprim);
        dispatch_params.maxprims_permsgroup = topologyconstants<primitive_t>::maxprims_permsgroup;

        resource_bindings bindings;
        bindings.constant = { 0, params.cbaddress };
        bindings.objectconstant = { 1, cbuffer.get_gpuaddress() };
        bindings.vertex = { 3, get_vertexbuffer_gpuaddress() };
        bindings.pipelineobjs = foundpso->second;
        bindings.rootconstants.slot = 2;
        bindings.rootconstants.values.resize(sizeof(dispatch_params));

        memcpy(bindings.rootconstants.values.data(), &dispatch_params, sizeof(dispatch_params));
        dispatch(bindings, params.wireframe, gfx::getmat(getparams().matname).ex());
    }

    template<typename body_t, topology primitive_t>
    inline void body_dynamic<body_t, primitive_t>::update_constbuffer()
    {
        objectconstants objconsts = { matrix::CreateTranslation(body.gcenter()), getview(), getmat(getparams().matname) };
        cbuffer.set_data(&objconsts);
    }

    template<typename body_t, topology primitive_t>
    inline void body_dynamic<body_t, primitive_t>::update_vertexbuffer()
    {
        update_perframebuffer(m_vertexbuffer_databegin, m_vertices.data(), get_vertexbuffersize());
    }

    template<typename body_t, topology primitive_t>
    inline D3D12_GPU_VIRTUAL_ADDRESS body_dynamic<body_t, primitive_t>::get_vertexbuffer_gpuaddress() const
    {
        return get_perframe_gpuaddress(m_vertexbuffer->GetGPUVirtualAddress(), get_vertexbuffersize());
    }
}