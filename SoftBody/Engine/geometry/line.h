#pragma once

#include "geocore.h"
#include "Engine/EngineUtils.h"

#include <optional>

namespace geometry
{
    struct line2D
    {
        line2D() = default;
        constexpr line2D(vec2 const& _point, vec2 const& _dir) : point(_point), dir(_dir) {}

        static float getparameter(line2D const& line, vec2 const& point);
        vec2 point, dir;
    };

    struct line
    {
        line() = default;
        constexpr line(vec3 const& _point, vec3 const& _dir) : point(_point), dir(_dir) {}

        line2D to2D() const;
        static float getparameter(line const& line, vec3 const& point);
        static std::optional<vec3> intersect_lines(line const& l, line const& r);
        vec3 point, dir;
    };

    struct linesegment
    {
        linesegment() = default;
        constexpr linesegment(vec3 const& _v0, vec3 const& _v1) : v0(_v0), v1(_v1){}

        static std::optional<vec2> intersect_line2D(linesegment const& linesegment, line const& line);
        vec3 v0, v1;
    };
}