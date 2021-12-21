#pragma once

#include "stdx/stdx.h"
#include "engine/simplemath.h"
#include "engine/engineutils.h"
#include "engine/graphics/gfxcore.h"
#include "geocore.h"
#include "beziermaths.h"

#include <array>
#include <vector>

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
        
        box gbox() const { return box; }
        aabb const& gaabb() const { return box; }
        aabb aabbworld() const { return box.move(center); }
        vector3 const& gcenter() const { return center; }
        vector3 const& gvelocity() const { return velocity; }
        void svelocity(vector3 const& vel) { velocity = vel; }
        std::vector<vector3> boxvertices() const { return gbox().gvertices(); }
        std::vector<vertex> const& gvertices() const { return evaluated_verts; }
        std::vector<vector3> const& physx_triangles() const { return physx_verts; }
        std::vector<uint8_t> const& texturedata() const { return {}; }

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

        aabb box;
        vector3 center = {};
        vector3 velocity = {};
        float restsize = 0.f;
        std::vector<vector3> physx_verts;
        std::vector<vertex> evaluated_verts;
        std::vector<vertex> vertices;

        static constexpr uint dim = 2;
        beziermaths::beziervolume<dim> volume;

        std::array<vector3, beziermaths::beziervolume<dim>::numcontrolpts> rest_config;
        std::array<vector3, beziermaths::beziervolume<dim>::numcontrolpts> velocities = {};
    };
}