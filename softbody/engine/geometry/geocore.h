#pragma once

#include "engine/core.h"
#include "stdx/stdx.h"
#include "engine/simplemath.h"

#include <vector>
#include <optional>

namespace geometry
{
    struct nullshape {};

    struct vertex
    {
        vector3 position = {};
        vector3 normal = {};
        vector2 texcoord = {};

        constexpr vertex() = default;
        constexpr vertex(vector3 const& pos, vector3 const& norm, vector2 txcoord = {}) : position(pos), normal(norm), texcoord(txcoord) {}
    };

    struct box
    {
        box() = default;
        box(vector3 const& _center, vector3 const& _extents) : center(_center), extents(_extents) {}

        std::vector<vector3> vertices();
        vector3 center, extents;
    };

    struct aabb
    {
        aabb() : min_pt(vector3::Zero), max_pt(vector3::Zero) {}
        aabb(vector3 const (&tri)[3]);

        aabb(std::vector<vector3> const& points);
        aabb(vector3 const* points, uint len);
        aabb(vector3 const& _min, vector3 const& _max) : min_pt(_min), max_pt(_max) {}

        vector3 center() const { return (max_pt + min_pt) / 2.f; }
        vector3 span() const { return max_pt - min_pt; }
        operator box() const { return { center(), span() }; }
        aabb move(vector3 const& off) const { return aabb(min_pt + off, max_pt + off); }

        aabb& operator+=(vector3 const& pt);
        std::optional<aabb> intersect(aabb const& r) const;

        // top left front = min, bot right back = max
        vector3 min_pt;
        vector3 max_pt;
    };
}