#pragma once

#include "engine/simplemath.h"
#include "engine/engineutils.h"
#include "engine/graphics/gfxcore.h"
#include "geocore.h"

#include <array>
#include <vector>

import shapes;

namespace geometry
{
    template <typename t>
    concept shapeffd_c = requires(t v)
    {
        v.generate_triangles();
        v.scenter(vec3{});
        {v.gcenter()} -> std::convertible_to<vec3>;
        {v.triangles()} -> std::convertible_to<std::vector<vertex>>;
    };

    class ffd_object
    {
    public:
        ffd_object(shapeffd_c auto object);

        uint to1D(uint i, uint j, uint k) const { return i + k * (l + 1) + j * (l + 1) * (m + 1); }
        std::vector<vertex> const& gvertices() const { return evaluated_verts; }
        std::vector<vec3> const& physx_triangles() const { return physx_verts; }
        
        box gbox() const { return box; }
        aabb const& gaabb() const { return box; }
        aabb aabbworld() const { return box.move(center); }
        vec3 const& gcenter() const { return center; }
        vec3 const& gvelocity() const { return velocity; }
        void svelocity(vec3 const& vel) { velocity = vel; }
        std::vector<vec3> boxvertices() const { return gbox().gvertices(); }

        void move(vec3 delta);
        void update(float dt);
        vec3 compute_wholebody_forces() const;
        std::vector<vec3> controlpoint_visualization() const;
        void resolve_collision(ffd_object &r, float dt);
        void resolve_collision_interior(aabb const& r, float dt);
        uint closest_controlpoint(vec3 point) const;
        std::vector<vec3> compute_contacts(ffd_object const&) const;
        vec3 compute_contact(ffd_object const&) const;
        std::vector<gfx::instance_data> controlnet_instancedata() const;
        vec3 evalbez_trivariate(float s, float t, float u) const;
        vec3 eval_bez_trivariate(float s, float t, float u) const;

        static vec3 parametric_coordinates(vec3 const& cartesian_coordinates, vec3 const& span);

    private:
        static constexpr uint l = 2, m = 2, n = 2;
        static std::size_t constexpr num_control_points = (l + 1) * (m + 1) * (n + 1);

        aabb box;
        vec3 center = {};
        vec3 velocity = {};
        float restsize = 0.f;
        std::vector<vec3> physx_verts;
        std::vector<vertex> evaluated_verts;
        // todo : this doesn't need to store normal
        std::vector<vertex> vertices;

        std::array<vec3, num_control_points> velocities;
        std::array<vec3, num_control_points> rest_config;
        std::array<vec3, num_control_points> control_points;

        static_assert(ffd_object::num_control_points > 0);
    };

    inline ffd_object::ffd_object(shapeffd_c auto object) : center(object.gcenter())
    {
        object.scenter(vec3::Zero);
        object.generate_triangles();

        evaluated_verts = object.triangles();

        auto const num_verts = evaluated_verts.size();

        vertices.reserve(num_verts);
        physx_verts.reserve(num_verts);

        for (auto const& vert : evaluated_verts)
        {
            box += vert.position;
            physx_verts.emplace_back(vert.position + center);
        }

        auto const& span = box.span();
        for (auto const& vert : evaluated_verts)
            vertices.push_back({ parametric_coordinates(vert.position, span), vert.normal });

        static constexpr uint product_ln = (l + 1) * (n + 1);
        for (uint idx = 0; idx < num_control_points; ++idx)
        {
            float const j = std::floorf(static_cast<float>(idx / product_ln));
            float const k = std::floorf(static_cast<float>((idx % product_ln) / (m + 1)));
            float const i = std::floorf(idx - k * (l + 1) - j * product_ln);

            // calculate in range [-span/2, span/2]
            rest_config[idx] = vec3{ span.x * ((i / l) - 0.5f), span.y * ((j / m) - 0.5f), span.z * ((k / n) - 0.5f) };
            control_points[idx] = rest_config[idx];
        }

        restsize = span.Length();
        std::fill(std::begin(velocities), std::end(velocities), vec3::Zero);
    }
}