#pragma once

#include "SimpleMath.h"
#include "DirectXMath.h"
#include "Graphics/gfxcore.h"
#include "EngineUtils.h"

#include <array>
#include <vector>
#include <limits>
#include <cmath>
#include <optional>
#include <algorithm>
#include <functional>

namespace Geometry
{
    using Vector2 = DirectX::SimpleMath::Vector2;
    using Vector3 = DirectX::SimpleMath::Vector3;
    using Vector4 = DirectX::SimpleMath::Vector4;
    using Matrix = DirectX::SimpleMath::Matrix;
    using Plane = DirectX::SimpleMath::Plane;

    struct nullshape {};

    struct Vertex
    {
        Vector3 position;
        Vector3 normal;

        Vertex() = default;

        constexpr Vertex(Vector3 const& pos, Vector3 const& norm)
            : position(pos)
            , normal(norm)
        {}
    };

    struct line2D
    {
        line2D() = default;
        constexpr line2D(Vector2 const& _point, Vector2 const& _dir)
            : point(_point), dir(_dir)
        {}

        static float getparameter(line2D const& line, Vector2 const& point);
        Vector2 point, dir;
    };

    struct line
    {
        line() = default;
        constexpr line(Vector3 const& _point, Vector3 const& _dir)
            : point(_point), dir(_dir)
        {}

        line2D to2D() const;
        static float getparameter(line const& line, Vector3 const& point);
        static std::optional<Vector3> intersect_lines(line const& l, line const& r);
        Vector3 point, dir;
    };

    struct linesegment
    {
        linesegment() = default;
        constexpr linesegment(Vector3 const& _v0, Vector3 const& _v1)
            : v0(_v0), v1(_v1)
        {}

        static std::optional<Vector2> intersect_line2D(linesegment const& linesegment, line const& line);
        Vector3 v0, v1;
    };

    struct triangle2D
    {
        triangle2D() = default;
        constexpr triangle2D(Vector2 const& _v0, Vector2 const& _v1, Vector2 const& _v2)
            : v0(_v0), v1(_v1), v2(_v2)
        {}

        bool isin(Vector2 const& point) const;

        Vector2 v0, v1, v2;

    };
    struct triangle
    {
        triangle() = default;
        constexpr triangle(Vector3 const& _v0, Vector3 const& _v1, Vector3 const& _v2)
            : v0(_v0), v1(_v1), v2(_v2)
        {}

        operator Plane() const { return { v0, v1, v2 }; }
        bool isin(Vector3 const &point) const;

        static std::optional<linesegment> intersect(triangle const& t0, triangle const& t1);
        Vector3 v0, v1, v2;
    };

    struct sphere
    {
        sphere() { generate_triangles(); }
        sphere(Vector3 const& _position, float _radius) : position(_position), radius(_radius) { generate_triangles(); }

        void generate_triangles();
        std::vector<Vertex> const& get_triangles() const;
        std::vector<linesegment> intersect(sphere const& r) const;
        static std::vector<linesegment> intersect(sphere const& l, sphere const& r);

    private:
        void addquad(float theta, float phi,float step_theta, float step_phi);
        Vertex create_spherevertex(float const phi, float const theta);

        std::vector<Vertex> triangulated_sphere;
    public:

        // todo : use num segments instead
        float step_degrees = 30.f;
        float radius = 1.5f;
        Vector3 position = {};
    };

    struct circle
    {
        std::vector<Vertex> get_triangles() const;
        std::vector<gfx::instance_data> get_instance_data() const;
        float step = 1.27f;
        float radius = 1.f;
        Vector3 center = {};
        Vector3 normal = {};
    };

    struct aabb
    {
        aabb() = default;

        constexpr aabb(Vector3 const& _min, Vector3 const& _max)
            : min_pt(_min), max_pt(_max)
        {}

        aabb(std::vector<Vector3> const& points)
        {
            min_pt = Vector3{ std::numeric_limits<float>::max() };
            max_pt = Vector3{ std::numeric_limits<float>::min() };

            // TODO : For larger set better to compute min separately for each set of ordinate
            for (auto const pt : points)
            {
                if (pt.x < min_pt.x)
                {
                    min_pt.x = pt.x;
                }
                if (pt.y < min_pt.y)
                {
                    min_pt.y = pt.y;
                }
                if (pt.z < min_pt.z)
                {
                    min_pt.z = pt.z;
                }

                if (pt.x > max_pt.x)
                {
                    max_pt.x = pt.x;
                }
                if (pt.y > max_pt.y)
                {
                    max_pt.y = pt.y;
                }
                if (pt.z > max_pt.z)
                {
                    max_pt.z = pt.z;
                }
            }
        }

        Vector3 span() const { return max_pt - min_pt; }

        // Top left back is min, bot right front max
        Vector3 min_pt;
        Vector3 max_pt;
    };

    class ffd_object
    {
        sphere ball;
    public:
        ffd_object() = default;

        ffd_object(sphere const& _sphere) : ball(_sphere), center(_sphere.position)
        {
            ball.position = Vector3::Zero;
            ball.generate_triangles();

            std::vector<Vertex> const &sphereverts = ball.get_triangles();

            auto const num_verts = sphereverts.size();
            std::vector<Vector3> verts;
            verts.reserve(num_verts);
            vertices.reserve(num_verts);
            for (auto const & vert : sphereverts)
            {
                verts.push_back(vert.position);
            }

            span = aabb(verts).span();
            
            for (auto const& vert : sphereverts)
            {
                vertices.push_back({ get_parametric_coordinates(vert.position), vert.normal });
                evaluated_verts.push_back({ vert.position + center, vert.normal });
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

                rest_config.push_back(Vector3{ span.x * (float(i) / float(l)), span.y * (float(j) / float(m)), span.z * (float(k) / float(n)) });
                control_points.push_back(box_minpt() + rest_config[idx] );
            }

            velocities.resize(num_control_points);
        }

        void update(float dt);

        inline Vector3 box_minpt() const { return center - span / 2.f; }

        Vector3 get_parametric_coordinates(Vector3 const& cartesian_coordinates) const
        {
            Vector3 const to_point = cartesian_coordinates + span / 2.f;
            
            // We can just use span as the deformation volume is axis aligned
            return { to_point.Dot(Vector3::UnitX) / span.x,
                to_point.Dot(Vector3::UnitY) / span.y,
                to_point.Dot(Vector3::UnitZ) / span.z };
        }

        uint get_1D_idx(uint i, uint j, uint k) const
        {
            return i + k * (l + 1) + j * (l + 1) * (m + 1);
        }

        void apply_force(Vector3 const& force, float dt)
        {
            velocity += force * dt;
        }

        void apply_force_at_controlpoint(uint i, uint j, uint k, Vector3 const& force)
        {
            apply_force_at_controlpoint(get_1D_idx(i, j, k), force);
        }

        void apply_force_at_controlpoint(uint ctrlpt_idx, Vector3 const& force)
        {
            velocities[ctrlpt_idx] += force;
        }

        std::vector<Vector3> get_control_point_visualization() const;

        std::vector<Vector3> const& get_control_net() const 
        { 
            return control_points; 
        }

        std::vector<Vertex> const& get_vertices() const
        {
            return evaluated_verts;
        }

        float fact(uint i) const
        {
            if (i < 1)
                return 1;

            return fact(i - 1) * i;
        }

        Vector3 eval_bez_trivariate(float s, float t, float u) const
        {
            Vector3 result = Vector3::Zero;
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

        Vector3 compute_wholebody_forces() const;
        void set_velocity(Vector3 const vel);
        void resolve_collision(ffd_object &r, float dt);
        uint closest_controlpoint(Vector3 const &point) const;
        std::vector<linesegment> intersect(ffd_object const& r) const;
        std::vector<Geometry::Vector3> compute_contacts(ffd_object const&) const;

        float penetration_duration;
        Vector3 span;
        Vector3 center = {};
        Vector3 accel = {};
        Vector3 velocity = {};
        std::vector<Vertex> evaluated_verts;
        std::vector<Vertex> vertices;
        std::vector<Vector3> control_points;
        std::vector<Vector3> rest_config;
        std::vector<Vector3> velocities;

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

namespace utils
{
    Geometry::Vector4 create_Vector4(Geometry::Vector3 const& v, float w = 1.f);
    Geometry::Matrix get_planematrix(Geometry::Vector3 translation, Geometry::Vector3 normal);
    std::vector<Geometry::Vector3> create_marker(Geometry::Vector3 point, float scale);
    std::vector<Geometry::Vector3> flatten(std::vector<Geometry::linesegment> const& lines);
    bool are_equal(float const& l, float const& r);
    bool are_equal(Geometry::Vector3 const& l, Geometry::Vector3 const& r);
}