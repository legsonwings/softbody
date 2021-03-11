#pragma once

#include "SimpleMath.h"
#include "DirectXMath.h"

#include <array>
#include <vector>
#include <limits>
#include <cmath>
#include <algorithm>
#include <functional>

namespace Geometry
{
    using Vector3 = DirectX::SimpleMath::Vector3;
    using Vector4 = DirectX::SimpleMath::Vector4;
    using Matrix = DirectX::SimpleMath::Matrix;
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

    struct sphere
    {
        std::vector<Vertex> const& GetTriangles();
    private:
        void GenerateTriangles();
        Vertex CreateSphereVertex(float const phi, float const theta);
        std::vector<Vertex> triangulated_sphere;
    public:
        sphere() = default;
        sphere(Vector3 const& _position) : position(_position)
        {}
        float step_degrees = 20.f;
        float radius = 1.5f;
        Vector3 position = {};
    };

    struct circle
    {
        std::vector<Vertex> GetTriangles();

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

        // Top left back is min, bot right front max
        Vector3 min_pt;
        Vector3 max_pt;
    };

    class ffd_object
    {
        sphere ball;
    public:
        ffd_object() = default;
        ffd_object(sphere const& _sphere) : ball(_sphere)
        {
            std::vector<Vertex> const &sphereverts = ball.GetTriangles();
            
            std::vector<Vector3> verts;
            verts.reserve(sphereverts.size());
            for (auto const & vert : sphereverts)
            {
                verts.push_back(vert.position);
            }

            box = aabb(verts);

            Vector3 const& span = box.max_pt - box.min_pt;
            unsigned const product_ln = (l + 1) * (n + 1);
            float const reciprocal_product_ln = 1.f / product_ln;

            unsigned num_control_pts = product_ln * (m + 1);
            control_points.reserve(num_control_pts);

            for (unsigned idx = 0; idx < num_control_pts; ++idx)
            {
                unsigned const j = static_cast<unsigned>(idx * reciprocal_product_ln);
                unsigned const k = static_cast<unsigned>((idx % product_ln) / (m + 1));
                unsigned const i = static_cast<unsigned>(idx - k * (l + 1) - j * product_ln);

                control_points.push_back(box.min_pt + Vector3{ span.x * (float(i) / float(l)), span.y * (float(j) / float(m)), span.z * (float(k) / float(n)) });
            }

            rest_config = control_points;
            velocities.resize(num_control_pts);

            for (auto const& vert : sphereverts)
            {
                vertices.push_back({ get_parametric_coordinates(vert.position), vert.normal });
            }
        }
        void update(float dt);
        Vector3 to_local(Vector3 const & point) const
        {
            return point - box.min_pt;
        }

        Vector3 to_global(Vector3 const& local_point) const
        {
            return box.min_pt + local_point;
        }

        Vector3 get_parametric_coordinates(Vector3 const& cartesian_coordinates) const
        {
            Vector3 to_point = cartesian_coordinates - box.min_pt;
            
            // We can just use span as the deformation volume is axis aligned
            Vector3 const& span = box.max_pt - box.min_pt;

            return { to_point.Dot(Vector3::UnitX) / span.x,
                to_point.Dot(Vector3::UnitY) / span.y,
                to_point.Dot(Vector3::UnitZ) / span.z };

        }

        unsigned get_1D_idx(unsigned i, unsigned j, unsigned k) const
        {
            return i + k * (l + 1) + j * (l + 1) * (m + 1);
        }

        void apply_force(unsigned i, unsigned j, unsigned k, Vector3 const & force)
        {
            std::vector<Vertex> ret;

            auto ctr_pt_idx = get_1D_idx(i, j, k);
            control_points[ctr_pt_idx] = control_points[ctr_pt_idx] + force;
        }

        std::vector<Vector3> get_control_point_visualization() const;
        std::vector<Vector3> const& get_control_net() const { return control_points; }

        std::vector<Vertex> get_vertices() const
        {
            std::vector<Vertex> result;
            result.reserve(vertices.size());

            for (auto const & vert : vertices)
            {
                result.push_back({ eval_bez_trivariate(vert.position.x, vert.position.y, vert.position.z), vert.normal });
            }

            return result;
        }

        float fact(unsigned i) const
        {
            if (i < 1)
                return 1;

            return fact(i - 1) * i;
        }

        Vector3 eval_bez_trivariate(float s, float t, float u) const
        {
            Vector3 result = Vector3::Zero;
            for (unsigned i = 0; i <= 2; ++i)
            {
                float basis_s = (float(l) / float(fact(i) * fact(l - i))) * std::powf(float(1 - s), float(l - i)) * std::powf(float(s), float(i));
                for (unsigned j = 0; j <= 2; ++j)
                {
                    float basis_t = (float(m) / float(fact(j) * fact(m - j))) * std::powf(float(1 - t), float(m - j)) * std::powf(float(t), float(j));
                    for (unsigned k = 0; k <= 2; ++k)
                    {
                        float basis_u = (float(n) / float(fact(k) * fact(n - k))) * std::powf(float(1 - u), float(n - k)) * std::powf(float(u), float(k));

                        result += basis_s * basis_t * basis_u * control_points[get_1D_idx(i, j, k)];
                    }
                }
            }

            return result;
        }

        aabb box;
        std::vector<Vertex> vertices;
        std::vector<Vector3> control_points;
        std::vector<Vector3> rest_config;
        std::vector<Vector3> velocities;

        unsigned l = 2, m = 2, n = 2;
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

    namespace Utils
    {
        Vector4 create_Vector4(Vector3 const& v, float w = 1.f);
        Matrix get_planematrix(Vector3 translation, Vector3 normal);
        std::vector<Vector3> create_marker(Vector3 point, float scale);
    }
}