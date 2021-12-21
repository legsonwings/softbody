#pragma once

#include "gfxcore.h"
#include "engine/sharedconstants.h"
#include "engine/geometry/geocore.h"
#include "engine/simplemath.h"
#include "engine/engineutils.h"
#include "engine/interfaces/bodyinterface.h"
#include "gfxmemory.h"
#include "resources.hpp"

#include <type_traits>
#include <unordered_map>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace gfx
{
    // todo : ensure these constraints can be satisfied by reference type instantiations
    // until then cannot constrain body templates
    template <typename t>
    concept sbody_c = requires(t v)
    {
        {v.gcenter()} -> std::convertible_to<vector3>;
        v.gvertices();
        {v.instancedata()} -> std::same_as<std::vector<instance_data>>;
    };

    template <typename t>
    concept dbody_c = requires(t v)
    {
        {v.gcenter()} -> std::convertible_to<vector3>;
        v.gvertices();
        v.update(float{});
    };

    template<topology prim_t = topology::triangle>
    struct topologyconstants
    {
        using vertextype = geometry::vertex;
        static constexpr uint32_t numverts_perprim = 3;
        static constexpr uint32_t maxprims_permsgroup = MAX_TRIANGLES_PER_GROUP;
    };

    template<>
    struct topologyconstants<topology::line>
    {
        using vertextype = vector3;
        static constexpr uint32_t numverts_perprim = 2;
        static constexpr uint32_t maxprims_permsgroup = MAX_LINES_PER_GROUP;
    };

    template<typename body_t, topology prim_t>
    class body_static : public bodyinterface
    {
        using rawbody_t = std::decay_t<body_t>;
        using vertextype = typename topologyconstants<prim_t>::vertextype;

        body_t body;
        staticbuffer<vertextype> _vertexbuffer;
        dynamicbuffer<instance_data> _instancebuffer;

        using vertexfetch_r = std::vector<vertextype>;
        using vertexfetch = std::function<vertexfetch_r(rawbody_t const&)>;
        using instancedatafetch_r = std::vector<instance_data>;
        using instancedatafetch = std::function<instancedatafetch_r(rawbody_t const&)>;

        vertexfetch get_vertices;
        instancedatafetch get_instancedata;

    public:
        body_static(rawbody_t _body, bodyparams const& _params);
        body_static(body_t const& _body, vertexfetch_r(rawbody_t::* vfun)() const, instancedatafetch_r(rawbody_t::* ifun)() const, bodyparams const& _params);

        std::vector<ComPtr<ID3D12Resource>> create_resources() override;
        void render(float dt, renderparams const&) override;
        const geometry::aabb getaabb() const;

        constexpr body_t& get() { return body; }
        constexpr body_t const& get() const { return body; }
        constexpr body_t& operator*() { return body; }
        constexpr body_t const& operator*() const { return body; }
        constexpr rawbody_t* operator->() { return &body; }
        constexpr rawbody_t const* operator->() const { return &body; }
    };

    template<typename body_t, topology prim_t>
    class body_dynamic : public bodyinterface
    {
        using rawbody_t = std::decay_t<body_t>;
        using vertextype = typename topologyconstants<prim_t>::vertextype;

        body_t body;
        constant_buffer cbuffer;
        dynamicbuffer<vertextype> _vertexbuffer;
        texture _texture{ 0, DXGI_FORMAT_R8G8B8A8_UNORM };

        using vertexfetch_r = std::vector<vertextype>;
        using vertexfetch = std::function<vertexfetch_r (rawbody_t const&)>;

        vertexfetch get_vertices;

        void update_constbuffer();
        uint vertexbuffersize() const { return _vertexbuffer.size(); }
    public:
        body_dynamic(rawbody_t _body, bodyparams const& _params, stdx::vecui2 texdims = {});
        body_dynamic(body_t const& _body, vertexfetch_r (rawbody_t::*fun)() const, bodyparams const& _params, stdx::vecui2 texdims = {});

        gfx::resourcelist create_resources() override;

        void update(float dt) override;
        void render(float dt, renderparams const&) override;

        constexpr body_t &get() { return body; } 
        constexpr body_t const &get() const { return body; }
        constexpr body_t &operator*() { return body; }
        constexpr body_t const &operator*() const { return body; }
        constexpr rawbody_t *operator->() { return &body; }
        constexpr rawbody_t const *operator->() const { return &body; }
    };
}

#include "body.hpp"