#pragma once

#include "Engine/SimpleMath.h"
#include "Engine/Graphics/gfxcore.h"
#include "Engine/engineutils.h"
#include "geocore.h"
#include "sphere.h"

#include <vector>

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
            velocities.resize(num_control_points);
        }

        void update(float dt);

        static vec3 get_parametric_coordinates(vec3 const& cartesian_coordinates, vec3 const &span)
        {
            vec3 const to_point = cartesian_coordinates + span / 2.f;
            
            // assume the volume is axis aligned and coordinates relative to volume center)
            return { to_point.x / span.x, to_point.y / span.y, to_point.z / span.z };
        }

        uint get_1D_idx(uint i, uint j, uint k) const
        {
            return i + k * (l + 1) + j * (l + 1) * (m + 1);
        }

        void apply_force(vec3 const& force, float dt)
        {
            velocity += force * dt;
        }

        void apply_force_at_controlpoint(uint i, uint j, uint k, vec3 const& force)
        {
            apply_force_at_controlpoint(get_1D_idx(i, j, k), force);
        }

        void apply_force_at_controlpoint(uint ctrlpt_idx, vec3 const& force)
        {
            velocities[ctrlpt_idx] += force;
        }

        std::vector<vec3> get_control_point_visualization() const;
        std::vector<gfx::instance_data> get_controlnet_instancedata() const;

        std::vector<vertex> const& get_vertices() const
        {
            return evaluated_verts;
        }

        std::vector<vec3> const& get_physx_traingles() const
        {
            return physx_verts;
        }

        float fact(uint i) const
        {
            if (i < 1)
                return 1;

            return fact(i - 1) * i;
        }

        vec3 eval_bez_trivariate(float s, float t, float u) const
        {
            vec3 result = vec3::Zero;
            for (uint i = 0; i <= 2; ++i)
            {
                float basis_s = (float(l) / float(fact(i) * fact(l - i))) * std::powf(float(1 - s), float(l - i)) * std::powf(float(s), float(i));
                for (uint j = 0; j <= 2; ++j)
                {
                    float basis_t = (float(m) / float(fact(j) * fact(m - j))) * std::powf(float(1 - t), float(m - j)) * std::powf(float(t), float(j));
                    for (uint k = 0; k <= 2; ++k)
                    {
                        float basis_u = (float(n) / float(fact(k) * fact(n - k))) * std::powf(float(1 - u), float(n - k)) * std::powf(float(u), float(k));

                        result += basis_s * basis_t * basis_u * control_points[get_1D_idx(i, j, k)];
                    }
                }
            }

            return result;
        }

        box getbox() const;
        aabb const& getboundingbox() const;
        std::vector<vec3> getbox_vertices() const;
        vec3 compute_wholebody_forces() const;
        void set_velocity(vec3 const vel);
        void resolve_collision(ffd_object &r, float dt);
        uint closest_controlpoint(vec3 const &point) const;
        std::vector<linesegment> intersect(ffd_object const& r) const;
        std::vector<vec3> compute_contacts(ffd_object const&) const;

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
    };

    struct Spring
    {
        Spring() = default;
        Spring(float _k, float _rl, int _m1, int _m2)
            : k(_k), rl(_rl), m1(_m1), m2(_m2)
        {}

        float k = 1.f;
        float rl = 0.f;

        int m1;
        int m2;

        float l = 0.f;
        float v = 0.f;
    };
}