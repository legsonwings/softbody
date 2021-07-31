#include "stdafx.h"
#include "geoutils.h"
#include "triangle.h"

using namespace DirectX;
using namespace geometry;

bool triangle2D::isin(vec2 const& point) const
{
    vec2 vec0 = v1 - v0, vec1 = v2 - v0, vec2 = point - v0;
    float den = vec0.x * vec1.y - vec1.x * vec0.y;
    float v = (vec2.x * vec1.y - vec1.x * vec2.y) / den;
    float w = (vec0.x * vec2.y - vec2.x * vec0.y) / den;
    float u = 1.0f - v - w;

    return (u >= 0.f && u <= 1.f) && (v >= 0.f && v <= 1.f) && (w >= 0.f && w <= 1.f);
}

bool geometry::triangle::isin(vec3 const* tri, vec3 const& point)
{
    vec3 normal_unnormalized = (tri[1] - tri[0]).Cross(tri[2] - tri[0]);
    vec3 normal = normal_unnormalized;
    normal.Normalize();

    float const triarea = normal.Dot(normal_unnormalized);
    if (triarea == 0)
        return false;

    float const alpha = normal.Dot((tri[1] - point).Cross(tri[2] - point)) / triarea;
    float const beta = normal.Dot((tri[2] - point).Cross(tri[0] - point)) / triarea;

    return alpha > -geoutils::tolerance<float> && beta > -geoutils::tolerance<float> && (alpha + beta) < (1.f + geoutils::tolerance<float>);
}

std::optional<linesegment> triangle::intersect(triangle const& t0, triangle const& t1)
{
    return intersect(t0.verts, t1.verts);
}

std::optional<linesegment> geometry::triangle::intersect(vec3 const* t0, vec3 const* t1)
{
    vec3 t0_normal = (t0[1] - t0[0]).Cross(t0[2] - t0[0]);
    vec3 t1_normal = (t1[1] - t1[0]).Cross(t1[2] - t1[0]);
    t0_normal.Normalize();
    t1_normal.Normalize();

    // construct third plane as cross product of the two plane normals and passing through origin
    auto linedir = t0_normal.Cross(t1_normal);
    linedir.Normalize();

    // use 3 plane intersection to find a point on the inerseciton line
    auto const det = matrix(t0_normal, t1_normal, linedir).Determinant();

    if (fabs(det) < FLT_EPSILON)
        return {};

    vec3 const& linepoint0 = (t0[0].Dot(t0_normal) * (t1_normal.Cross(linedir)) + t1[0].Dot(t1_normal) * linedir.Cross(t0_normal)) / det;

    line const isect_line{ linepoint0, linedir };

    std::vector<line> edges;
    edges.reserve(6);

    auto dir = (t0[1] - t0[0]);
    dir.Normalize();
    edges.emplace_back(line{ t0[0], dir });
    dir = (t0[2] - t0[1]);
    dir.Normalize();
    edges.emplace_back(line{ t0[1], dir });
    dir = (t0[0] - t0[2]);
    dir.Normalize();
    edges.emplace_back(line{ t0[2], dir });

    dir = (t1[1] - t1[0]);
    dir.Normalize();
    edges.emplace_back(line{ t1[0], dir });
    dir = (t1[2] - t1[1]);
    dir.Normalize();
    edges.emplace_back(line{ t1[1], dir });
    dir = (t1[0] - t1[2]);
    dir.Normalize();
    edges.emplace_back(line{ t1[2], dir });

    std::vector<vec3> isect_linesegment;
    isect_linesegment.reserve(2);

    for (auto const& edge : edges)
    {
        // ignore intersections with parallel edge
        if (geoutils::nearlyequal(std::fabsf(edge.dir.Dot(isect_line.dir)), 1.f))
            continue;

        // the points on intersection of two triangles will be on both triangles
        if (auto isect = line::intersect_lines(edge, isect_line); isect.has_value() && triangle::isin(t0, isect.value()) && triangle::isin(t1, isect.value()))
            isect_linesegment.push_back(isect.value());
    }

    // remove duplicates, naively, maybe improve this
    for (uint i = 0; i < std::max(uint(1), isect_linesegment.size()) - 1; ++i)
        for (uint j = i + 1; j < isect_linesegment.size(); ++j)
            if (geoutils::nearlyequal(isect_linesegment[i], isect_linesegment[j]))
            {
                std::swap(isect_linesegment[j], isect_linesegment[isect_linesegment.size() - 1]);
                isect_linesegment.pop_back();
                j--;
            }

    if (isect_linesegment.size() < 2)
        return {};

    //assert(isect_linesegment.size() != 2);

    // there is an issue here
    /*if (isect_linesegment.size() != 2)
    {
        DebugBreak();
    }*/

    return { { isect_linesegment[0], isect_linesegment[1] } };
}