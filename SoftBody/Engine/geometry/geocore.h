#pragma once

#include "geodefines.h"
#include "Engine/SimpleMath.h"
#include "engine/stdx.h"

#include <vector>
#include <optional>

namespace geometry
{
    struct nullshape {};

    struct vertex
    {
        vec3 position = {};
        vec3 normal = {};

        constexpr vertex() = default;
        constexpr vertex(vec3 const& pos, vec3 const& norm) : position(pos), normal(norm){}
    };

    struct box
    {
        box() = default;
        box(vec3 const& _center, vec3 const& _extents) : center(_center), extents(_extents) {}

        std::vector<vec3> gvertices();
        vec3 center, extents;
    };

    struct aabb
    {
        aabb() : min_pt(vec3::Zero), max_pt(vec3::Zero) {}
        aabb(vec3 const (&tri)[3]);

        aabb(std::vector<vec3> const& points);
        aabb(vec3 const* points, uint len);
        aabb(vec3 const& _min, vec3 const& _max) : min_pt(_min), max_pt(_max) {}

        vec3 center() const { return (max_pt + min_pt) / 2.f; }
        vec3 span() const { return max_pt - min_pt; }
        operator box() const { return { center(), span() }; }
        aabb move(vec3 const& off) const { return aabb(min_pt + off, max_pt + off); }

        aabb& operator+=(vec3 const& pt);
        std::optional<aabb> intersect(aabb const& r) const;

        // Top left front = min, bot right back = max
        vec3 min_pt;
        vec3 max_pt;
    };
}