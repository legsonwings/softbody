#pragma once

#include "stdx/stdx.h"
#include "engine/simplemath.h"
#include "engine/engineutils.h"
#include "engine/graphics/gfxcore.h"
#include "geocore.h"
#include "beziermaths.h"

#include <array>
#include <vector>
#include <cstdint>

import shapes;

namespace geometry
{
    template <typename t>
    concept shapeffd_c = requires(t v)
    {
        v.generate_triangles();
        v.scenter(vector3{});
        {v.gcenter()} -> std::convertible_to<vector3>;
        {v.triangles()} -> std::convertible_to<std::vector<vertex>>;
    };

    struct ffddata
    {
        vector3 center;
        std::vector<vertex> vertices;
    };

    class ffd_object
    {
    public:
        ffd_object(ffddata data);
        
        box box() const { return _box; }
        aabb const& bbox() const { return _box; }
        aabb bboxworld() const { return _box.move(_center); }
        vector3 const& center() const { return _center; }
        vector3 const& velocity() const { return _velocity; }
        void svelocity(vector3 const& vel) { _velocity = vel; }
        std::vector<vector3> boxvertices() const { return box().vertices(); }
        std::vector<vertex> const& vertices() const { return _evaluated_verts; }
        std::vector<vector3> const& physx_triangles() const { return _physx_verts; }
        std::vector<uint8_t> const& texturedata() const { static std::vector<uint8_t> r(4); return r; }

        void move(vector3 delta);
        void update(float dt);
        vector3 compute_wholebodyforces() const;
        vector3 compute_contact(ffd_object const&) const;
        std::vector<vector3> compute_contacts(ffd_object const&) const;
        void resolve_collision(ffd_object& r, float dt);
        void resolve_collision_interior(aabb const& r, float dt);
        uint closest_controlpoint(vector3 point) const;
        std::vector<vector3> controlpoint_visualization() const;
        std::vector<gfx::instance_data> controlnet_instancedata() const;
        vector3 eval_bez_trivariate(float s, float t, float u) const;

        static vector3 parametric_coordinates(vector3 const& cartesian_coordinates, vector3 const& span);

    private:

        aabb _box;
        vector3 _center = {};
        vector3 _velocity = {};
        float _restsize = 0.f;
        std::vector<vector3> _physx_verts;
        std::vector<vertex> _evaluated_verts;
        std::vector<vertex> _vertices;

        static constexpr uint dim = 2;
        beziermaths::beziervolume<dim> _volume;

        std::array<vector3, beziermaths::beziervolume<dim>::numcontrolpts> _rest_config;
        std::array<vector3, beziermaths::beziervolume<dim>::numcontrolpts> _velocities = {};
    };
}