#include "stdafx.h"
#include "geoutils.h"

#include "DirectXMath.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace geometry;

vec4 geoutils::create_vec4(vec3 const& v, float w)
{
    return { v.x, v.y, v.z, w };
}

matrix geoutils::get_planematrix(vec3 translation, vec3 normal)
{
    vec3 axis1 = normal.Cross(vec3::UnitX);
    if (axis1.LengthSquared() <= 0.001)
    {
        axis1 = normal.Cross(vec3::UnitY);
    }

    axis1.Normalize();

    vec3 axis2 = axis1.Cross(normal);
    axis2.Normalize();

    // treat normal as y axis
    Matrix result = Matrix(axis1, axis2, -normal);
    result.Translation(translation);

    return result;
}

std::vector<vec3> geoutils::create_marker(vec3 point, float scale)
{
    auto apply_transformation = [](std::vector<vec3> const& points, const vec3(&tx)[2])
    {
        std::vector<vec3> transformed_points;
        transformed_points.reserve(points.size());

        auto const angle = tx[0].Length();
        auto axis = tx[0];
        axis.Normalize();

        for (auto const& point : points)
        {
            vec3 result;
            vec3::Transform(point, DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(axis, XMConvertToRadians(angle)), result);
            result += tx[1];

            transformed_points.push_back(std::move(result));
        }

        return transformed_points;
    };

    auto create_unit_marker = [&apply_transformation]()
    {
        static constexpr vec3 transformations[6][2] =
        {
            {{0.f, 0.f, 360.f}, {0.f, 0.0f, 0.5f}},
            {{0.f, 0.f, 360.f}, {0.f, 0.0f, -0.5f}},
            {{90.f, 0.f, 0.f}, {0.f, 0.5f, 0.f}},
            {{90.f, 0.f, 0.f}, {0.f, -0.5f, 0.f}},
            {{0.f, 90.f, 0.f}, {0.5f, 0.f, 0.f}},
            {{0.f, 90.f, 0.f}, {-0.5f, 0.0f, 0.f}}
        };

        static std::vector<vec3> unit_frontface_quad =
        {
            { -0.5f, 0.5f, 0.f },
            { 0.5f, 0.5f, 0.f },
            { 0.5f, -0.5f, 0.f },
            { -0.5f, -0.5f, 0.f }
        };

        std::vector<vec3> unit_marker_lines;
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

    static std::vector<vec3> unit_marker = create_unit_marker();

    std::vector<vec3> result;
    result.reserve(unit_marker.size());
    for (auto const vertex : unit_marker)
    {
        result.push_back((vertex * scale) + point);
    }

    return result;
}

bool geoutils::are_equal(float const& l, float const& r)
{
    return std::fabsf(l - r) < tolerance<float>;
}

bool geoutils::are_equal(vec3 const& l, vec3 const& r)
{
    return std::fabsf(l.x - r.x) < tolerance<float> && std::fabsf(l.y - r.y) < tolerance<float> && std::fabsf(l.z - r.z) < tolerance<float>;
}