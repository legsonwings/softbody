#include "body.h"
#include "gfxutils.h"
#include "sharedconstants.h"

#include <string>
#include <functional>

namespace gfx
{
    using default_and_upload_buffers = std::pair<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>>;

    void dispatch(resource_bindings const &bindings);
    default_and_upload_buffers create_vertexbuffer_default(void* vertexdata_start, std::size_t const vb_size);
    ComPtr<ID3D12Resource> createupdate_uploadbuffer(uint8_t** mapped_buffer, void* data_start, std::size_t const perframe_buffersize);
    D3D12_GPU_VIRTUAL_ADDRESS get_perframe_gpuaddress(D3D12_GPU_VIRTUAL_ADDRESS start, UINT64 perframe_buffersize);
    void update_perframebuffer(uint8_t* mapped_buffer, void* data_start, std::size_t const perframe_buffersize);
    
    template<typename geometry_t, topology primitive_t>
    template<typename>
    inline body_static<geometry_t, primitive_t>::body_static(geometry_t _body, bodyparams const& _params) : bodyinterface(_params), body(_body)
    {
        get_vertices = [](geometry_t const& geom) { return geom.get_vertices(); };
    }

    template<typename geometry_t, topology primitive_t>
    inline body_static<geometry_t, primitive_t>::body_static(geometry_t const& _body, vertexfetch_r (std::decay_t<geometry_t>::* vfun)() const, instancedatafetch_r (std::decay_t<geometry_t>::* ifun)() const, bodyparams const& _params) : bodyinterface(_params), body(_body)
    {
        // todo : add support for data member pointers
        get_vertices = [vfun](geometry_t const& geom) { return std::invoke(vfun, geom); };
        get_instancedata = [ifun](geometry_t const& geom) { return std::invoke(ifun, geom); };
    }

    template<typename geometry_t, topology primitive_t>
    std::vector<ComPtr<ID3D12Resource>> body_static<geometry_t, primitive_t>::create_resources()
    {
        m_vertices = get_vertices(body);
        auto const instances = get_instancedata(body);
        num_instances = instances.size();
        m_cpu_instance_data.reset(new instance_data[num_instances]);

        std::copy(instances.begin(), instances.end(), m_cpu_instance_data.get());

        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);

        auto const vb_resources = create_vertexbuffer_default(m_vertices.data(), get_vertexbuffersize());
        m_vertexbuffer = vb_resources.first;

        m_instance_buffer = createupdate_uploadbuffer(&m_instancebuffer_mapped, m_cpu_instance_data.get(), get_instancebuffersize());

        // return the upload buffer so that engine can keep it alive until vertex data has been uploaded to gpu
        return { vb_resources.second };
    }

    template<typename geometry_t, topology primitive_t>
    inline void body_static<geometry_t, primitive_t>::render(float dt, renderparams const &params)
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

        dispatch_params.numverts_perprim = topologyconstants::numverts_perprim<primitive_t>;
        dispatch_params.numprims_perinstance = static_cast<uint32_t>(get_numvertices() / topologyconstants::numverts_perprim<primitive_t>);
        dispatch_params.numprims = static_cast<uint32_t>(get_numinstances() * dispatch_params.numprims_perinstance);
        dispatch_params.maxprims_permsgroup = topologyconstants::maxprims_permsgroup<primitive_t>;

        resource_bindings bindings;
        bindings.constant = { 0, params.cbaddress };
        bindings.vertex = { 3, get_vertexbuffer_gpuaddress() };
        bindings.instance = { 4, get_instancebuffer_gpuaddress() };
        bindings.root_signature = foundpso->second.root_signature;
        bindings.pso = foundpso->second.pso;
        bindings.rootconstants.slot = 2;
        bindings.rootconstants.values.resize(sizeof(dispatch_params));

        memcpy(bindings.rootconstants.values.data(), &dispatch_params, sizeof(dispatch_params));
        dispatch(std::move(bindings));
    }

    template<typename geometry_t, topology primitive_t>
    inline D3D12_GPU_VIRTUAL_ADDRESS body_static<geometry_t, primitive_t>::get_instancebuffer_gpuaddress() const
    {
        return get_perframe_gpuaddress(m_instance_buffer->GetGPUVirtualAddress(), get_instancebuffersize());
    }

    template<typename geometry_t, topology primitive_t>
    inline void body_static<geometry_t, primitive_t>::update_instancebuffer()
    {
        auto const instances = get_instancedata(body);
        std::copy(instances.begin(), instances.end(), m_cpu_instance_data.get());
        update_perframebuffer(m_instancebuffer_mapped, m_cpu_instance_data.get(), get_instancebuffersize());
    }

    template<typename geometry_t, topology primitive_t>
    template<typename>
    inline body_dynamic<geometry_t, primitive_t>::body_dynamic(geometry_t _body, bodyparams const& _params)  : bodyinterface(_params), body(_body)
    {
        get_vertices = [](geometry_t const& geom) { return geom.get_vertices(); };
    }

    template<typename geometry_t, topology primitive_t>
    inline body_dynamic<geometry_t, primitive_t>::body_dynamic(geometry_t const& _body, vertexfetch_r (std::decay_t<geometry_t>::*fun)() const, bodyparams const& _params) : bodyinterface(_params), body(_body)
    {
        // todo : add support for data member pointers and any callable(lambdas, functors)
        get_vertices = [fun](geometry_t const& geom) { return std::invoke(fun, geom); };
    }

    template<typename geometry_t, topology primitive_t>
    inline std::vector<ComPtr<ID3D12Resource>> body_dynamic<geometry_t, primitive_t>::create_resources()
    {
        m_vertices = get_vertices(body);
        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);

        cbuffer.createresources<objectconstants>();
        m_vertexbuffer = createupdate_uploadbuffer(&m_vertexbuffer_databegin, m_vertices.data(), get_vertexbuffersize());
        return std::vector<ComPtr<ID3D12Resource>>();
    }

    template<typename geometry_t, typename = std::enable_if_t<std::is_lvalue_reference_v<geometry_t>>>
    inline void update_body(std::type_identity_t<std::decay_t<geometry_t>> const& body, float dt) {}

    template<typename geometry_t>
    inline void update_body(std::type_identity_t<std::decay_t<geometry_t>> &body, float dt) { body.update(dt); }

    template<typename geometry_t, topology primitive_t>
    inline void body_dynamic<geometry_t, primitive_t>::update(float dt)
    {
        // todo : find a better way to organize bits only used by value/reference type geometry_t
        update_body<geometry_t>(body, dt);
        m_vertices = get_vertices(body);
        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);
    }

    template<typename geometry_t, topology primitive_t>
    inline void body_dynamic<geometry_t, primitive_t>::render(float dt, renderparams const &params)
    {
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

        dispatch_params.numverts_perprim = topologyconstants::numverts_perprim<primitive_t>;
        dispatch_params.numprims = static_cast<uint32_t>(get_numvertices() / topologyconstants::numverts_perprim<primitive_t>);
        dispatch_params.maxprims_permsgroup = topologyconstants::maxprims_permsgroup<primitive_t>;

        resource_bindings bindings;
        bindings.constant = { 0, params.cbaddress };
        bindings.objectconstant = { 1, cbuffer.get_gpuaddress() };
        bindings.vertex = { 3, get_vertexbuffer_gpuaddress() };
        bindings.root_signature = foundpso->second.root_signature;
        bindings.pso = foundpso->second.pso;
        bindings.rootconstants.slot = 2;
        bindings.rootconstants.values.resize(sizeof(dispatch_params));

        memcpy(bindings.rootconstants.values.data(), &dispatch_params, sizeof(dispatch_params));
        dispatch(std::move(bindings));
    }

    template<typename geometry_t, topology primitive_t>
    inline void body_dynamic<geometry_t, primitive_t>::update_constbuffer()
    {
        // todo : vertices are already in world space so create matrix at origin
        objectconstants objconsts = { matrix::CreateTranslation(vec3::Zero), gfx::getmat(getparams().matname)};
        cbuffer.set_data(&objconsts);
    }

    template<typename geometry_t, topology primitive_t>
    inline void body_dynamic<geometry_t, primitive_t>::update_vertexbuffer()
    {
        update_perframebuffer(m_vertexbuffer_databegin, m_vertices.data(), get_vertexbuffersize());
    }

    template<typename geometry_t, topology primitive_t>
    inline D3D12_GPU_VIRTUAL_ADDRESS body_dynamic<geometry_t, primitive_t>::get_vertexbuffer_gpuaddress() const
    {
        return get_perframe_gpuaddress(m_vertexbuffer->GetGPUVirtualAddress(), get_vertexbuffersize());
    }
}