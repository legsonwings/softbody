#include "stdafx.h"
#include "Shapes.h"
#include <iterator>

using namespace DirectX;
using namespace Geometry;

std::vector<Vertex> const& Geometry::sphere::GetTriangles()
{
    GenerateTriangles();
    return triangulated_sphere;
}

void Geometry::sphere::GenerateTriangles()
{
    float step_radians = DirectX::XMConvertToRadians(step_degrees);
    unsigned num_vertices = static_cast<unsigned>((XM_PI / step_radians) * (XM_2PI / step_radians));

    triangulated_sphere.reserve(num_vertices);

    // Divide the sphere into quads for each region defined by two circles(at theta and theta + step)
    for (float theta = 0.f; theta < (XM_PI - step_radians + 0.0001f); theta += step_radians)
    {
        for (float phi = 0.f; phi < (XM_2PI - step_radians + 0.0001f); phi += step_radians)
        {
            // Create the quad(two triangles) using the vertices
            Vertex lbv, ltv, rbv, rtv;
            ltv = CreateSphereVertex(phi, theta);
            rtv = CreateSphereVertex(phi + step_radians, theta);
            lbv = CreateSphereVertex(phi, theta + step_radians);
            rbv = CreateSphereVertex(phi + step_radians, theta + step_radians);

            if (lbv.position != ltv.position && lbv.position != rtv.position && ltv.position != rtv.position)
            {
                triangulated_sphere.push_back(lbv);
                triangulated_sphere.push_back(ltv);
                triangulated_sphere.push_back(rtv);
            }

            if (lbv.position != rtv.position && lbv.position != rbv.position && rtv.position != rbv.position)
            {
                triangulated_sphere.push_back(lbv);
                triangulated_sphere.push_back(rtv);
                triangulated_sphere.push_back(rbv);
            }
        }
    }

}

Vertex sphere::CreateSphereVertex(float const phi, float const theta)
{
    Vertex res;
    res.position = { sinf(theta) * cosf(phi), cosf(theta), sinf(theta) * sinf(phi) };
    res.normal = res.position;
    res.position *= radius;
    return res;
}

void Geometry::ffd_object::update(float dt)
{
    for (std::size_t ctrl_pt_idx = 0; ctrl_pt_idx < control_points.size(); ++ctrl_pt_idx)
    {
        auto const omega = 5.f; //std::sqrt(spring.k / blob_physx[0].mass);
        auto const eta = 0.5f;

        auto const alpha = omega * std::sqrt(1 - eta * eta);
        auto displacement = control_points[ctrl_pt_idx] - rest_config[ctrl_pt_idx];

        // Damped SHM
        auto delta_pos = std::powf(2.71828f, -omega * eta * dt) * (displacement * std::cos(alpha * dt) + (velocities[ctrl_pt_idx] + omega * eta * displacement) * std::sin(alpha * dt) / alpha);
        velocities[ctrl_pt_idx] = -std::pow(2.71828f, -omega * eta * dt) * ((displacement * omega * eta - velocities[ctrl_pt_idx] - omega * eta * displacement) * cos(alpha * dt) +
            (displacement * alpha + (velocities[ctrl_pt_idx] + omega * eta * displacement) * omega * eta / alpha) * sin(alpha * dt));

        control_points[ctrl_pt_idx] = rest_config[ctrl_pt_idx] + delta_pos;
    }
}

std::vector<Vector3> Geometry::ffd_object::get_control_point_visualization() const
{
    auto const marker_vec3 = Utils::create_marker(Vector3::Zero, 0.1f);

    std::vector<Vector3> marker;
    marker.reserve(marker_vec3.size());
    for (auto const& point : marker_vec3)
    {
        marker.push_back(point);
    }

    return marker;
}

std::vector<Vertex> Geometry::circle::GetTriangles()
{
    std::vector<Vector3> vertices;

    float x0 = radius * cosf(0);
    float y0 = radius * sinf(0);

    for (float t = step; t <= (XM_2PI + step); t+=step)
    {
        float x = radius * cosf(t);
        float y = radius * sinf(t);

        vertices.push_back(Vector3::Zero);
        vertices.push_back({x, y , 0.f});
        vertices.push_back({x0, y0 , 0.f});

        x0 = x;
        y0 = y;
    }

    Matrix plane_tx = Utils::get_planematrix(center, normal);

    std::vector<Vertex> result;
    for (auto& vert : vertices)
    {
       // result.push_back({vert, normal });
        result.push_back({ Vector3::Transform(vert, plane_tx), normal });
    }

    return result;
}


Vector4 Geometry::Utils::create_Vector4(Vector3 const& v, float w)
{
    return { v.x, v.y, v.z, w };
}

Matrix Geometry::Utils::get_planematrix(Vector3 translation, Vector3 normal)
{
    Vector3 axis1 = normal.Cross(Vector3::UnitX);
    if (axis1.LengthSquared() <= 0.001)
    {
        axis1 = normal.Cross(Vector3::UnitY);
    }

    axis1.Normalize();

    Vector3 axis2 = axis1.Cross(normal);
    axis2.Normalize();

    // treat normal as y axis
    Matrix result = Matrix(axis1, axis2, -normal);
    result.Translation(translation);

    return result;
}

std::vector<Geometry::Vector3> Geometry::Utils::create_marker(Vector3 point, float scale)
{
    auto apply_transformation = [](std::vector<Vector3> const& points, const Vector3(&tx)[2])
    {
        std::vector<Geometry::Vector3> transformed_points;
        transformed_points.reserve(points.size());

        auto const angle = tx[0].Length();
        auto axis = tx[0];
        axis.Normalize();

        for (auto const& point : points)
        {
            Vector3 result;
            Vector3::Transform(point, DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(axis, XMConvertToRadians(angle)), result);
            result += tx[1];

            transformed_points.push_back(std::move(result));
        }

        return transformed_points;
    };

    auto create_unit_marker = [&apply_transformation]()
    {
        static constexpr Vector3 transformations[6][2] =
        {
            {{0.f, 0.f, 360.f}, {0.f, 0.0f, 0.5f}},
            {{0.f, 0.f, 360.f}, {0.f, 0.0f, -0.5f}},
            {{90.f, 0.f, 0.f}, {0.f, 0.5f, 0.f}},
            {{90.f, 0.f, 0.f}, {0.f, -0.5f, 0.f}},
            {{0.f, 90.f, 0.f}, {0.5f, 0.f, 0.f}},
            {{0.f, 90.f, 0.f}, {-0.5f, 0.0f, 0.f}}
        };

        static std::vector<Vector3> unit_frontface_quad =
        {
            { -0.5f, 0.5f, 0.f },
            { 0.5f, 0.5f, 0.f },
            { 0.5f, -0.5f, 0.f },
            { -0.5f, -0.5f, 0.f }
        };

        std::vector<Geometry::Vector3> unit_marker_lines;
        for (auto const& transformation : transformations)
        {
            auto const face = apply_transformation(unit_frontface_quad, transformation);
            auto const num_verts = face.size();
            for (size_t vert_idx = 0; vert_idx < num_verts; ++vert_idx)
            {
                unit_marker_lines.push_back(face[vert_idx]);
                unit_marker_lines.push_back(face[(vert_idx + 1) % num_verts]);
            }
        }

        return unit_marker_lines;
    };

    static std::vector<Vector3> unit_marker = create_unit_marker();

    std::vector<Geometry::Vector3> result;
    result.reserve(unit_marker.size());
    for (auto const vertex : unit_marker)
    {
        result.push_back((vertex * scale) + point);
    }

    return result;
}
