#include "stdafx.h"
#include "geoutils.h"

#include "DirectXMath.h"
#include "Engine/engineutils.h"

#include <array>

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace geometry;

vec4 geoutils::create_vec4(vec3 const& v, float w)
{
    return { v.x, v.y, v.z, w };
}

matrix geoutils::get_planematrix(vec3 const &translation, vec3 const &normal)
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

std::vector<vec3> geoutils::create_box(vec3 const &center, vec3 const &extents)
{
    auto apply_transformation = [](const vec3(&verts)[4], const vec3(&tx)[3])
    {
        std::array<vec3, 4> transformed_points;

        auto const scale = tx[2];
        auto const angle = tx[0].Length();
        auto axis = tx[0];
        axis.Normalize();

        for (uint i = 0; i < 4; ++i)
        {
            vec3 result = { verts[i].x * scale.x, verts[i].y * scale.y, verts[i].z * scale.z };
            vec3::Transform(result, DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(axis, XMConvertToRadians(angle)), result);

            transformed_points[i] = std::move(result + tx[1]);
        }

        return transformed_points;
    };

    auto create_marker = [&apply_transformation](auto extents)
    {
        auto const scale = vec3{ extents.x / 2.f, extents.y / 2.f, extents.z / 2.f };
        vec3 transformations[6][3] =
        {
            {{0.f, 0.f, 360.f}, {0.f, 0.0f, scale.z}, {scale}},
            {{0.f, 0.f, 360.f}, {0.f, 0.0f, -scale.z}, {scale}},
            {{90.f, 0.f, 0.f}, {0.f, scale.y, 0.f}, {scale}},
            {{90.f, 0.f, 0.f}, {0.f, -scale.y, 0.f}, {scale}},
            {{0.f, 90.f, 0.f}, {scale.x, 0.f, 0.f}, {scale}},
            {{0.f, 90.f, 0.f}, {-scale.x, 0.0f, 0.f}, {scale}}
        };

        static constexpr vec3 unit_frontface_quad[4] =
        {
            { -1.f, 1.f, 0.f },
            { 1.f, 1.f, 0.f },
            { 1.f, -1.f, 0.f },
            { -1.f, -1.f, 0.f }
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

    std::vector<vec3> unit_marker = create_marker(extents);

    std::vector<vec3> result;
    result.reserve(unit_marker.size());
    for (auto const vertex : unit_marker)
    {
        result.push_back((vertex) + center);
    }

    return result;
}

std::vector<vec3> geoutils::create_cube(vec3 const& center, float scale)
{
    return create_box(center, { scale, scale, scale });
}

bool geoutils::are_equal(float const& l, float const& r)
{
    return std::fabsf(l - r) < tolerance<float>;
}

bool geoutils::are_equal(vec3 const& l, vec3 const& r)
{
    return std::fabsf(l.x - r.x) < tolerance<float> && std::fabsf(l.y - r.y) < tolerance<float> && std::fabsf(l.z - r.z) < tolerance<float>;
}