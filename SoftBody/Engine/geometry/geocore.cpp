#include "stdafx.h"
#include "geocore.h"
#include "geoutils.h"

using namespace DirectX::SimpleMath;
using namespace geometry;

std::vector<vec3> geometry::box::get_vertices()
{
    return geoutils::create_box(center, extents);
}

aabb::aabb(std::vector<vec3> const& points)
{
    min_pt = vec3{ std::numeric_limits<float>::max() };
    max_pt = vec3{ std::numeric_limits<float>::lowest() };

    for (auto const& pt : points)
    {
        min_pt.x = std::min(pt.x, min_pt.x);
        min_pt.y = std::min(pt.y, min_pt.y);
        min_pt.z = std::min(pt.z, min_pt.z);

        max_pt.x = std::max(pt.x, max_pt.x);
        max_pt.y = std::max(pt.y, max_pt.y);
        max_pt.z = std::max(pt.z, max_pt.z);
    }
}