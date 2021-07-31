#include "stdafx.h"
#include "geocore.h"
#include "geoutils.h"

#include<algorithm>

using namespace DirectX::SimpleMath;
using namespace geometry;

std::vector<vec3> geometry::box::get_vertices()
{
    return geoutils::create_box_lines(center, extents);
}

geometry::aabb::aabb(vec3 const* tri)
{
    vec3 const *v0 = tri;
    vec3 const *v1 = tri + 1;
    vec3 const *v2 = tri + 2;

    min_pt.x = std::min({ v0->x, v1->x, v2->x });
    max_pt.x = std::max({ v0->x, v1->x, v2->x });
    min_pt.y = std::min({ v0->y, v1->y, v2->y });
    max_pt.y = std::max({ v0->y, v1->y, v2->y });
    min_pt.z = std::min({ v0->z, v1->z, v2->z });
    max_pt.z = std::max({ v0->z, v1->z, v2->z });
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

        min_pt -= geoutils::tolerance<vec3>;
        max_pt += geoutils::tolerance<vec3>;
    }
}

std::optional<aabb> geometry::aabb::intersect(aabb const& r) const
{
    if (max_pt.x < r.min_pt.x || min_pt.x > r.max_pt.x) return {};
    if (max_pt.y < r.min_pt.y || min_pt.y > r.max_pt.y) return {};
    if (max_pt.z < r.min_pt.z || min_pt.z > r.max_pt.z) return {};

    vec3 const min = { std::max(min_pt.x, r.min_pt.x), std::max(min_pt.y, r.min_pt.y), std::max(min_pt.z, r.min_pt.z) };
    vec3 const max = { std::min(max_pt.x, r.max_pt.x), std::min(max_pt.y, r.max_pt.y), std::min(max_pt.z, r.max_pt.z) };

    return { {min, max} };
}
