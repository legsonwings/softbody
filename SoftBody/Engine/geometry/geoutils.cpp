#include "stdafx.h"
#include "geoutils.h"

#include "DirectXMath.h"
#include "Engine/engineutils.h"

#include <array>

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace geometry;

constexpr vec3 unit_frontface_quad[4] =
{
    { -1.f, 1.f, 0.f },
    { 1.f, 1.f, 0.f },
    { 1.f, -1.f, 0.f },
    { -1.f, -1.f, 0.f }
};

std::array<vertex, 4> transform_unitquad(const vec3(&verts)[4], const vec3(&tx)[3])
{
    static constexpr vec3 unitquad_normal = {0.f, 0.f, -1.f};
    std::array<vertex, 4> transformed_points;

    auto const scale = tx[2];
    auto const angle = XMConvertToRadians(tx[0].Length());
    auto axis = tx[0];
    axis.Normalize();

    for (uint i = 0; i < 4; ++i)
    {
        vec3 normal;
        vec3 pos = { verts[i].x * scale.x, verts[i].y * scale.y, verts[i].z * scale.z };
        vec3::Transform(pos, DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(axis, angle), pos);
        vec3::Transform(unitquad_normal, DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(axis, angle), normal);

        transformed_points[i] = std::move(vertex{(pos + tx[1]), normal});
    }

    return transformed_points;
};

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

std::vector<vertex> geoutils::create_cube(vec3 const& center, vec3 const& extents)
{
    auto const scale = vec3{ extents.x / 2.f, extents.y / 2.f, extents.z / 2.f };
    vec3 transformations[6][3] =
    {
        {{0.f, 180.f, 0.f}, {0.f, 0.0f, scale.z}, {scale}},
        {{0.f, 360.f, 0.f}, {0.f, 0.0f, -scale.z}, {scale}},
        {{90.f, 0.f, 0.f}, {0.f, scale.y, 0.f}, {scale}},
        {{-90.f, 0.f, 0.f}, {0.f, -scale.y, 0.f}, {scale}},
        {{0.f, -90.f, 0.f}, {scale.x, 0.f, 0.f}, {scale}},
        {{0.f, 90.f, 0.f}, {-scale.x, 0.0f, 0.f}, {scale}}
    };

    std::vector<vertex> triangles;
    for (auto const& transformation : transformations)
    {
        auto const face = transform_unitquad(unit_frontface_quad, transformation);
        assert(face.size() == 4);
        triangles.emplace_back(face[0]);
        triangles.emplace_back(face[1]);
        triangles.emplace_back(face[2]);
        triangles.emplace_back(face[2]);
        triangles.emplace_back(face[3]);
        triangles.emplace_back(face[0]);
    }

    for (auto& vertex : triangles) { vertex.position += center; }
    return triangles;
}

std::vector<vec3> geoutils::create_box_lines(vec3 const &center, vec3 const &extents)
{
    auto const scale = vec3{ extents.x / 2.f, extents.y / 2.f, extents.z / 2.f };
    vec3 transformations[6][3] =
    {
        {{0.f, 360.f, 0.f}, {0.f, 0.0f, scale.z}, {scale}},
        {{0.f, 180.f, 0.f}, {0.f, 0.0f, -scale.z}, {scale}},
        {{-90.f, 0.f, 0.f}, {0.f, scale.y, 0.f}, {scale}},
        {{90.f, 0.f, 0.f}, {0.f, -scale.y, 0.f}, {scale}},
        {{0.f, 90.f, 0.f}, {scale.x, 0.f, 0.f}, {scale}},
        {{0.f, -90.f, 0.f}, {-scale.x, 0.0f, 0.f}, {scale}}
    };

    std::vector<vec3> quads;
    for (auto const& transformation : transformations)
    {
        auto const face = transform_unitquad(unit_frontface_quad, transformation);
        auto const num_verts = face.size();
        for (size_t vert_idx = 0; vert_idx < num_verts; ++vert_idx)
        {
            quads.push_back(face[vert_idx].position);
            quads.push_back(face[(vert_idx + 1) % num_verts].position);
        }
    }

    for (auto & vertex : quads) { vertex += center; }
    return quads;
}

std::vector<vec3> geoutils::create_cube_lines(vec3 const& center, float scale)
{
    return create_box_lines(center, { scale, scale, scale });
}

bool geoutils::nearlyequal(arithmetic_c auto const& l, arithmetic_c auto const& r, float const& _tolerance) 
{ 
    return std::fabsf(l - r) < _tolerance;
}

bool geoutils::nearlyequal(vec2 const& l, vec2 const& r, float const& _tolerance)
{
    return nearlyequal(l.x, r.x, _tolerance) && nearlyequal(l.y, r.y, _tolerance);
}

bool geoutils::nearlyequal(vec3 const& l, vec3 const& r, float const& _tolerance)
{
    return nearlyequal(l.x, r.x, _tolerance) && nearlyequal(l.y, r.y, _tolerance) && nearlyequal(l.z, r.z, _tolerance);
}

bool geoutils::nearlyequal(vec4 const& l, vec4 const& r, float const& _tolerance)
{
    return nearlyequal(l.x, r.x, _tolerance) && nearlyequal(l.y, r.y, _tolerance) && nearlyequal(l.z, r.z, _tolerance) && nearlyequal(l.w, r.w, _tolerance);
}