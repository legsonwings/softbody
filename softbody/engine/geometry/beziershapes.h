#pragma once

#include "engine/stdx.h"
#include "beziermaths.h"
#include "engine/graphics/gfxcore.h"
#include "engine/graphics/gfxutils.h"

#include <vector>
#include <iterator>

namespace geometry
{
template<uint n, uint m>
struct beziershape
{
    beziermaths::beziertriangle<n> patches[m] = {};
    static constexpr uint gdgree() { return n; }
    static constexpr uint gnumpatches() { return m; }
    vector3 gcenter() const { return vector3::Zero; }
};

template<uint n>
std::vector<beziermaths::curveeval> tessellate(beziermaths::beziercurve<n> const& curve)
{
    // 10 divisions per unit length
    static constexpr float unitstep = 0.1f;
    float curvelen = 0.f;

    // approximate curve length
    for (uint i = 1; i < curve.numcontrolpts; ++i)
        curvelen += vector3::Distance(curve.controlnet[i - 1], curve.controlnet[i]);

    float const step = unitstep / curvelen;

    std::vector<beziermaths::curveeval> result;
    result.reserve(static_cast<uint>(curvelen / unitstep));
    result.push_back(evaluate(curve, 0.f));
    result.push_back(evaluate(curve, step));

    float const end = 2.f + 0.5f * step;
    for (float t = 2 * step; t <= end; t += step)
    {
        // add the line
        result.push_back(result.back());
        result.push_back(evaluate(curve, t));
    }

    return result;
}

template<uint n>
std::vector<geometry::vertex> tessellate(beziermaths::beziertriangle<n> const& patch)
{
    static constexpr int numRows = 16;
    static constexpr int numTotalTris = numRows * numRows;
    static constexpr float step = 1.f / numRows;

    std::vector<geometry::vertex> result;
    result.reserve(numTotalTris * 3);

    float end = 0.f + step - stdx::tolerance<>;
    for (int row = 0; row < numRows; ++row)
    {
        float topV, botV;
        topV = 1.f - row * step;
        botV = 1.f - ((row + 1) * step);
        vector3 topLeft = { 1.f - topV, topV, 0.f };
        vector3 topRight = { 0.f, topV, 1.f - topV };

        vector3 botLeft = { 1.f - botV, botV, 0.f };
        vector3 botRight = { 0.f, botV, 1.f - botV };

        // compute the first edge
        vector3 topVert = stdx::lerp(topLeft, topRight, 0.f);
        vector3 botLeftVert = stdx::lerp(botLeft, botRight, 0.f);

        auto lastEdge = std::make_pair<geometry::vertex>(evaluate(patch, botLeftVert), evaluate(patch, topVert));
        int const numRowTris = (2 * row + 1);

        // compute the triangle strip for each layer
        for (int triIdx = 0; triIdx < numRowTris; ++triIdx)
        {
            static const auto norm = vector3{ 0.f, 0.f, 1.f };
            if (triIdx % 2 == 0)
            {
                // skip intermediate triangles when calculating interpolation t which do not contribute in change of horizontal(uw) parameter span
                // in total there will be floor(nTris / 2) such triangles
                vector3 const botRightVert = stdx::lerp(botLeft, botRight, (float(triIdx - (triIdx / 2)) + 1.f) / (float(row) + 1.f));
                result.push_back(lastEdge.first);
                result.push_back(lastEdge.second);
                result.push_back(evaluate(patch, botRightVert));
                lastEdge = { lastEdge.second, result.back().vertices[2] };
            }
            else
            {
                // skip intermediate triangles when calculating interpolation t which do not contribute in change of horizontal(uw) parameter span
                // in total there will be floor(nTris / 2) such triangles
                vector3 const topRightVert = stdx::lerp(topLeft, topRight, (float(triIdx - (triIdx / 2))) / float(row));
                result.push_back(lastEdge.first);
                result.push_back(evaluate(patch, topRightVert));
                result.push_back(lastEdge.second);
                lastEdge = { lastEdge.second, result.back().vertices[1] };
            }
        }
    }

    return result;
}

template<uint n>
std::vector<vector3> tessellate(beziermaths::beziervolume<n> const& vol)
{
    static constexpr float unitstep = 0.7f;
    auto const& bbox = geometry::aabb(vol.controlnet.data(), vol.numcontrolpts);
    auto const span = bbox.span();

    assert(span.LengthSquared() > stdx::tolerance<>);

    std::vector<vector3> res;
    float const step = unitstep / std::max({ span.x, span.y, span.z });
    float const end = 1.f + step - stdx::tolerance<>;
    for (float i(0.f); i <= end; i += step)
        for (float j(0.f); j <= end; j += step)
            for (float k(0.f); k <= end; k += step)
                res.push_back(evaluatefast(vol, { i, j, k }).first);

    return res;
}

template<uint n, uint M>
std::vector<geometry::vertex> tessellateshape(beziershape<n, M> const& shape)
{
    std::vector<geometry::vertex> result;
    for (int i = 0; i < M; ++i)
    {
        auto tesselatedpatch = tessellate(shape.patches[i]);
        std::move(tesselatedpatch.begin(), tesselatedpatch.end(), back_inserter(result));
    }
    return result;
}

template<uint n>
std::vector<vector3> getwireframe_controlmesh(beziermaths::beziertriangle<n> const& patch)
{
    std::vector<vector3> result;
    for (int row = 0; row < n; ++row)
    {
        auto previousRowStart = row * (row + 1) / 2;
        auto nextRowStart = (row + 1) * (row + 2) / 2;
        auto previousRowEnd = previousRowStart + row;
        auto nextRowEnd = nextRowStart + row + 1;

        result.push_back(patch.controlnet[previousRowStart]);
        result.push_back(patch.controlnet[nextRowStart]);
        result.push_back(patch.controlnet[previousRowEnd]);
        result.push_back(patch.controlnet[nextRowEnd]);
        for (int nextRowPt = previousRowEnd + 1; nextRowPt < previousRowEnd + 1 + row + 1; ++nextRowPt)
        {
            // this is the line to next point in this row
            result.push_back(patch.controlnet[nextRowPt]);
            result.push_back(patch.patch.controlnet[nextRowPt + 1]);

            // these are lines from the next point to previous row(form inverted traingles)
            if (nextRowPt < previousRowEnd + 1 + row)
            {
                auto const startPtInPrevRow = previousRowStart + nextRowPt - (previousRowEnd + 1);
                result.push_back(patch.controlnet[nextRowPt + 1]);
                result.push_back(patch.controlnet[startPtInPrevRow]);
                result.push_back(patch.controlnet[nextRowPt + 1]);
                result.push_back(patch.controlnet[startPtInPrevRow + 1]);
            }
        }
    }
    return result;
}

struct qbeziercurve
{
    std::vector<vector3> gvertices() const 
    { 
        std::vector<vector3> res;
        for (auto const& v : tessellate(curve)) res.push_back(v.first);
        return res;
    }

    std::vector<gfx::instance_data> instancedata() const { return { gfx::instance_data(matrix::CreateTranslation(vector3::Zero), gfx::getview(), gfx::getmat("")) }; }

    beziermaths::beziercurve<2u> curve;
};

struct qbeziervolume
{
    std::vector<vector3> gvertices() const
    {
        static constexpr auto delta = 0.01f;
        static constexpr auto deltax = vector3{ delta, 0.f, 0.f };
        static constexpr auto deltay = vector3{ 0.f, delta, 0.f };
        static constexpr auto deltaz = vector3{ 0.f, 0.f, delta };

        auto const& volpoints = tessellate(vol);
        std::vector<vector3> res;
        res.reserve(volpoints.size() * 2);
        for (auto const& v : volpoints)
        {
            res.push_back(v); res.push_back(v + deltax);
            res.push_back(v); res.push_back(v - deltax);
            res.push_back(v); res.push_back(v + deltay);
            res.push_back(v); res.push_back(v - deltay);
            res.push_back(v); res.push_back(v + deltaz);
            res.push_back(v); res.push_back(v - deltaz);
        }
        return res;
    }

    std::vector<gfx::instance_data> instancedata() const { return { gfx::instance_data(matrix::CreateTranslation(vector3::Zero), gfx::getview(), gfx::getmat("")) }; }

    static qbeziervolume create(float len)
    {
        qbeziervolume unitvol;
        for (auto i : std::ranges::iota_view{ 0u, unitvol.vol.numcontrolpts })
        {
            auto const& idx = stdx::hypercubeidx<2>::from1d(2, i);
            unitvol.vol.controlnet[i] = len * vector3{ static_cast<float>(idx.coords[0]) / 2, static_cast<float>(idx.coords[2]) / 2 , static_cast<float>(idx.coords[1]) / 2 };
        }
        return unitvol;
    }

    beziermaths::beziervolume<2u> vol;
};
}