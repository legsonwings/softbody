#pragma once

#include "Engine/SimpleMath.h"
#include "Engine/Graphics/gfxcore.h"
#include "Engine/engineutils.h"
#include "geocore.h"
#include "geoutils.h"

#include <vector>

import shapes;

namespace geometry
{
    struct circle
    {
        std::vector<vertex> get_triangles() const;
        std::vector<gfx::instance_data> get_instance_data() const;
        float step = 1.27f;
        float radius = 1.f;
        vec3 center = {};
        vec3 normal = {};
    };

    class ffd_object
    {
    public:
        ffd_object() = default;

        uint get1D(uint i, uint j, uint k) const { return i + k * (l + 1) + j * (l + 1) * (m + 1); }
        std::vector<vertex> const& get_vertices() const { return evaluated_verts; }
        std::vector<vec3> const& get_physx_triangles() const { return physx_verts; }
        std::vector<vec3> get_control_point_visualization() const { return geoutils::create_cube_lines(vec3::Zero, 0.1f); }
        std::vector<gfx::instance_data> get_controlnet_instancedata() const;

        ffd_object(sphere const& _sphere) : ball(_sphere), center(_sphere.position)
        {
            ball.position = vec3::Zero;
            ball.generate_triangles();

            std::vector<vertex> const &sphereverts = ball.get_triangles();

            auto const num_verts = sphereverts.size();
            std::vector<vec3> verts;
            verts.reserve(num_verts);
            vertices.reserve(num_verts);
            physx_verts.reserve(num_verts);
            evaluated_verts.reserve(num_verts);
            for (auto const & vert : sphereverts)
            {
                verts.push_back(vert.position);
            }

            auto const &span = aabb(verts).span();
            auto const& min_pt = center - span / 2.f;

            for (auto const& vert : sphereverts)
            {
                vertices.push_back({ get_parametric_coordinates(vert.position, span), vert.normal });

                auto const &pos = vert.position + center;
                physx_verts.push_back(pos);
                evaluated_verts.push_back({ pos, vert.normal });
            }

            uint const product_ln = (l + 1) * (n + 1);
            float const reciprocal_product_ln = 1.f / product_ln;

            //uint num_ctrl_pts = product_ln * (m + 1);

            rest_config.reserve(num_control_points);
            control_points.reserve(num_control_points);

            for (uint idx = 0; idx < num_control_points; ++idx)
            {
                unsigned const j = static_cast<unsigned>(idx * reciprocal_product_ln);
                unsigned const k = static_cast<unsigned>((idx % product_ln) / (m + 1));
                unsigned const i = static_cast<unsigned>(idx - k * (l + 1) - j * product_ln);

                rest_config.push_back(vec3{ span.x * (float(i) / float(l)), span.y * (float(j) / float(m)), span.z * (float(k) / float(n)) });
                control_points.push_back(min_pt + rest_config[idx]);
            }

            box = std::move(aabb{ control_points });
            velocities.resize(control_points.size(), vec3::Zero);
        }

        void update(float dt);

        static vec3 get_parametric_coordinates(vec3 const& cartesian_coordinates, vec3 const &span)
        {
            vec3 const to_point = cartesian_coordinates + span / 2.f;
            
            // assume the volume is axis aligned and coordinates relative to volume center)
            return { to_point.x / span.x, to_point.y / span.y, to_point.z / span.z };
        }

        float fact(uint i) const
        {
            if (i < 1)
                return 1;

            return fact(i - 1) * i;
        }

        vec3 evalbez_trivariate(float s, float t, float u) const;
        vec3 eval_bez_trivariate(float s, float t, float u) const;
        
        void move(vec3 delta);
        vec3 const& getcenter() const { return center; }
        box getbox() const;
        aabb const& getaabb() const;
        std::vector<vec3> getbox_vertices() const;
        vec3 compute_wholebody_forces() const;
        vec3 const& getvelocity() const { return velocity; }
        void set_velocity(vec3 const& vel);
        void resolve_collision(ffd_object &r, float dt);
        void resolve_collision_interior(aabb const& r, float dt);
        uint closest_controlpoint(vec3 const &point) const;
        std::vector<vec3> compute_contacts(ffd_object const&) const;
        vec3 compute_contact(ffd_object const&) const;

    private:
        aabb box;
        sphere ball;
        vec3 center = {};
        vec3 velocity = {};
        std::vector<vec3> physx_verts;
        std::vector<vertex> evaluated_verts;
        std::vector<vertex> vertices;
        std::vector<vec3> control_points;
        std::vector<vec3> rest_config;
        std::vector<vec3> velocities;

        uint l = 2, m = 2, n = 2;
        static std::size_t constexpr num_control_points = 27;

        static_assert(ffd_object::num_control_points > 0);
    };
}