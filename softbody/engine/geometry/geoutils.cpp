#include "geoutils.h"

#include "DirectXMath.h"
#include "Engine/engineutils.h"

#include <array>

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace geometry;
using namespace stdx;

constexpr vector3 unit_frontface_quad[4] =
{
    { -1.f, 1.f, 0.f },
    { 1.f, 1.f, 0.f },
    { 1.f, -1.f, 0.f },
    { -1.f, -1.f, 0.f }
};

constexpr vector3 unitcube[8] =
{
    { -1.f, 1.f, -1.f },
    { 1.f, 1.f, -1.f },
    { 1.f, -1.f, -1.f },
    { -1.f, -1.f, -1.f },
    { -1.f, 1.f, 1.f },
    { 1.f, 1.f, 1.f },
    { 1.f, -1.f, 1.f },
    { -1.f, -1.f, 1.f }
};

std::array<vertex, 4> transform_unitquad(const vector3(&verts)[4], const vector3(&tx)[3])
{
    static constexpr vector3 unitquad_normal = {0.f, 0.f, -1.f};
    std::array<vertex, 4> transformed_points;

    auto const scale = tx[2];
    auto const angle = XMConvertToRadians(tx[0].Length());
    auto const axis = tx[0].Normalized();

    for (uint i = 0; i < 4; ++i)
    {
        vector3 normal;
        vector3 pos = { verts[i].x * scale.x, verts[i].y * scale.y, verts[i].z * scale.z };
        vector3::Transform(pos, DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(axis, angle), pos);
        vector3::Transform(unitquad_normal, DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(axis, angle), normal);

        transformed_points[i] = std::move(vertex{(pos + tx[1]), normal});
    }

    return transformed_points;
};

vector4 geoutils::create_vector4(vector3 const& v, float w)
{
    return { v.x, v.y, v.z, w };
}

matrix geoutils::get_planematrix(vector3 const &translation, vector3 const &normal)
{
    vector3 axis1 = normal.Cross(vector3::UnitX);
    if (axis1.LengthSquared() <= 0.001)
        axis1 = normal.Cross(vector3::UnitY);

    axis1.Normalize();

    vector3 const axis2 = axis1.Cross(normal).Normalized();

    // treat normal as y axis
    Matrix result = Matrix(axis1, axis2, -normal);
    result.Translation(translation);

    return result;
}

std::vector<vertex> geoutils::create_cube(vector3 const& center, vector3 const& extents)
{
    auto const scale = vector3{ extents.x / 2.f, extents.y / 2.f, extents.z / 2.f };
    vector3 transformations[6][3] =
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

std::vector<vector3> geoutils::create_box_lines(vector3 const &center, vector3 const &extents)
{
    auto createcubelines = [](vector3 const (&cube)[8])
    {
        std::vector<vector3> res;
        res.resize(24);

        for (uint i(0); i < 4; ++i)
        {
            // front face lines
            res[i * 2] = cube[i];
            res[i * 2 + 1] = cube[(i + 1) % 4];

            // back face lines
            res[i * 2 + 8] = cube[i + 4];
            res[i * 2 + 8 + 1] = cube[(i + 1) % 4 + 4];

            // lines connecting front and back faces
            res[16 + i * 2] = cube[i];
            res[16 + i * 2 + 1] = cube[i + 4];
        }

        return res;
    };

    static const std::vector<vector3> unitcubelines = createcubelines(unitcube);

    auto const scale = vector3{ extents.x / 2.f, extents.y / 2.f, extents.z / 2.f };
    
    std::vector<vector3> res;
    res.reserve(24);
    for (auto const v : unitcubelines) res.emplace_back(v.x * scale.x, v.y * scale.y, v.z * scale.z);
    return res;
}

std::vector<vector3> geoutils::create_cube_lines(vector3 const& center, float scale)
{
    return create_box_lines(center, { scale, scale, scale });
}

bool geoutils::nearlyequal(arithmeticpure_c auto const& l, arithmeticpure_c auto const& r, float const& _tolerance) 
{ 
    return std::fabsf(l - r) < _tolerance;
}

bool geoutils::nearlyequal(vector2 const& l, vector2 const& r, float const& _tolerance)
{
    return nearlyequal(l.x, r.x, _tolerance) && nearlyequal(l.y, r.y, _tolerance);
}

bool geoutils::nearlyequal(vector3 const& l, vector3 const& r, float const& _tolerance)
{
    return nearlyequal(l.x, r.x, _tolerance) && nearlyequal(l.y, r.y, _tolerance) && nearlyequal(l.z, r.z, _tolerance);
}

bool geoutils::nearlyequal(vector4 const& l, vector4 const& r, float const& _tolerance)
{
    return nearlyequal(l.x, r.x, _tolerance) && nearlyequal(l.y, r.y, _tolerance) && nearlyequal(l.z, r.z, _tolerance) && nearlyequal(l.w, r.w, _tolerance);
}