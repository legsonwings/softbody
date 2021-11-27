#include "body.h"
#include "gfxutils.h"
#include "engine/sharedconstants.h"

// todo :  remove after reafactor
#include "engine/interfaces/engineinterface.h"

#include <string>
#include <functional>

namespace gfx
{
    using default_and_upload_buffers = std::pair<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>>;
    
    ComPtr<ID3D12DescriptorHeap> createsrvdescriptorheap(D3D12_DESCRIPTOR_HEAP_DESC heapdesc);
    void createsrv(D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc, ComPtr<ID3D12Resource> resource, ComPtr<ID3D12DescriptorHeap> srvheap);
    void dispatch(resource_bindings const &bindings, bool wireframe = false, bool twosided = false, uint dispatchx = 1);
    default_and_upload_buffers create_vertexbuffer_default(void* vertexdata_start, std::size_t const vb_size);
    ComPtr<ID3D12Resource> createtexture_default(uint width, uint height);
    ComPtr<ID3D12Resource> createupdate_uploadbuffer(uint8_t** mapped_buffer, void* data_start, std::size_t const perframe_buffersize);
    uint updatesubres(ID3D12Resource* dest, ID3D12Resource* upload, const D3D12_SUBRESOURCE_DATA* srcdata);
    //ComPtr<ID3D12Resource> createupdate_uploadtexture(uint width, uint height, uint8_t** mappedtex, void* data_start, std::size_t const perframe_texsize);
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

        // todo : revise these as instancing has changed
        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);

        auto const vb_resources = create_vertexbuffer_default(m_vertices.data(), get_vertexbuffersize());
        _vertexbuffer = vb_resources.first;

        _instancebuffer = createupdate_uploadbuffer(&m_instancebuffer_mapped, m_instancedata.data(), get_instancebuffersize());

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

        uint const numasthreads = static_cast<uint>(std::ceil(static_cast<float>(dispatch_params.numprims) / static_cast<float>(INSTANCE_ASGROUP_SIZE * dispatch_params.maxprims_permsgroup)));

        memcpy(bindings.rootconstants.values.data(), &dispatch_params, sizeof(dispatch_params));
        dispatch(bindings, params.wireframe, gfx::getmat(getparams().matname).ex(), numasthreads);
    }

    template<typename body_t, topology primitive_t>
    inline const geometry::aabb body_static<body_t, primitive_t>::getaabb() const { return body.gaabb(); }

    template<typename body_t, topology primitive_t>
    inline D3D12_GPU_VIRTUAL_ADDRESS body_static<body_t, primitive_t>::get_instancebuffer_gpuaddress() const
    {
        return get_perframe_gpuaddress(_instancebuffer->GetGPUVirtualAddress(), get_instancebuffersize());
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
        D3D12_DESCRIPTOR_HEAP_DESC srvheapdesc = {};
        srvheapdesc.NumDescriptors = 1;
        srvheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        _srvheap = createsrvdescriptorheap(srvheapdesc);

        m_vertices = get_vertices(body);

        // todo : check if this is still valid
        assert(m_vertices.size() < AS_GROUP_SIZE * MAX_MSGROUPS_PER_ASGROUP * MAX_TRIANGLES_PER_GROUP * 3);

        cbuffer.createresources<objectconstants>();
        _vertexbuffer = createupdate_uploadbuffer(&_vertexbuffer_mapped, m_vertices.data(), get_vertexbuffersize());
        
        if (texturedatasize() > 0)
        {
            // todo : we don't need any mapped pointers for textures
            _textureupload = create_uploadbuffer(&_texture_mapped, configurable_properties::frame_count * texturedatasize());
            _texture = createtexture_default(getparams().texdims[0], getparams().texdims[1]);

            D3D12_SUBRESOURCE_DATA subresdata;
            subresdata.pData = _texturedata.data();
            subresdata.RowPitch = getparams().texdims[0] * 4;
            subresdata.SlicePitch = getparams().texdims[0] * getparams().texdims[1] * 4;

            updatesubres(_texture.Get(), _textureupload.Get(), &subresdata);

            //_texture->WriteToSubresource(0, &box, _texturedata.data(), box.right * 12, box.right * box.bottom * 12);

            D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {};
            srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvdesc.Texture2D.MipLevels = 1;

            // create srv into the descriptor heap
            createsrv(srvdesc, _texture, _srvheap);
        }

        return {};
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
        updatetexture();
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
        bindings.srvheap = _srvheap;

        if(texturedatasize() > 0)
            bindings.texture = { 5, 0 };

        memcpy(bindings.rootconstants.values.data(), &dispatch_params, sizeof(dispatch_params));
        dispatch(bindings, params.wireframe, gfx::getmat(getparams().matname).ex());
    }

    template<typename body_t, topology prim_t>
    inline void body_dynamic<body_t, prim_t>::texturedata(std::vector<uint8_t> texturedata)
    {
        _texturedata = std::move(texturedata);
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
        update_perframebuffer(_vertexbuffer_mapped, m_vertices.data(), get_vertexbuffersize());
    }

    template<typename body_t, topology prim_t>
    inline void body_dynamic<body_t, prim_t>::updatetexture()
    {
        if (texturedatasize() > 0)
        {
            D3D12_SUBRESOURCE_DATA subresdata;
            subresdata.pData = _texturedata.data();
            subresdata.RowPitch = getparams().texdims[0] * 4;
            subresdata.SlicePitch = getparams().texdims[0] * getparams().texdims[1] * 4;

            updatesubres(_texture.Get(), _textureupload.Get(), &subresdata);
        }
    }

    template<typename body_t, topology primitive_t>
    inline D3D12_GPU_VIRTUAL_ADDRESS body_dynamic<body_t, primitive_t>::get_vertexbuffer_gpuaddress() const
    {
        return get_perframe_gpuaddress(_vertexbuffer->GetGPUVirtualAddress(), get_vertexbuffersize());
    }

    template<typename body_t, topology prim_t>
    inline D3D12_GPU_VIRTUAL_ADDRESS body_dynamic<body_t, prim_t>::texturegpuaddress() const
    {
        return get_perframe_gpuaddress(_texture->GetGPUVirtualAddress(), texturedatasize());
    }
}