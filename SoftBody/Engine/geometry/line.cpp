#include "stdafx.h"
#include "line.h"
#include "geoutils.h"

using namespace DirectX;
using namespace geometry;

std::optional<vec2> linesegment::intersect_line2D(linesegment const& linesegment, line const& line)
{
    auto cross = [](vec2 const& l, vec2 const& r) { return l.x * r.y - l.y * r.x; };
    // point = p + td1
    // point = q + ud2

    vec2 p(linesegment.v0), q(line.point);
    vec2 d1(linesegment.v1 - linesegment.v0), d2(line.dir);
    d1.Normalize();

    if (float d1crossd2 = cross(d1, d2); d1crossd2 != 0.f)
    {
        float t = cross(q - p, d2) / d1crossd2;

        return p + t * d1;
    }

    return {};
}

line2D line::to2D() const
{
    return { point.To2D(), dir.To2D() };
}

float line2D::getparameter(line2D const& line, vec2 const& point)
{
    // assuming point is already on the line
    auto origin = line.point;
    auto topoint = point - origin;
    topoint.Normalize();

    return vec2::Distance(origin, point) * (topoint.Dot(line.dir) > 0.f ? 1.f : -1.f);
}

float line::getparameter(line const& line, vec3 const& point)
{
    // assuming point is already on the line
    auto origin = line.point;
    auto topoint = point - origin;
    topoint.Normalize();

    return vec3::Distance(origin, point) * (topoint.Dot(line.dir) > 0.f ? 1.f : -1.f);
}

std::optional<vec3> geometry::line::intersect_lines(line const& l, line const& r)
{
    // if the lines intersect they need to be on the same plane.
    // calculate plane normal
    auto n = l.dir.Cross(r.dir);
    n.Normalize();

    // if lines intersect dot product to normal and any point on both lines is constant
    auto const c1 = n.Dot(l.point);
    if (std::fabs(c1 - n.Dot(r.point)) > geoutils::tolerance<float>)
        return {};

    // calculate cross of n and line dir
    auto ldir_cross_n = l.dir.Cross(n);
    ldir_cross_n.Normalize();

    // this constant should be same for all points on l, i.e 
    auto const c = ldir_cross_n.Dot(l.point);

    // c = ldir_cross_n.Dot(isect) and isect = r.point + r.dir * t2
    auto const t2 = (c - ldir_cross_n.Dot(r.point)) / ldir_cross_n.Dot(r.dir);

    return { r.point + t2 * r.dir };
}
