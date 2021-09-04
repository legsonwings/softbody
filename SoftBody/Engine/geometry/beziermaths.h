#pragma once

#include "geodefines.h"
#include "geocore.h"
#include "geoutils.h"
#include "engine/stdx.h"
#include "engine/engineutils.h"
#include <array>
#include <vector>
#include <cstdint>
#include <utility>
#include <iterator>
#include <algorithm>

// todo : are all of the functions really constexpr?
namespace beziermaths
{
    using controlpoint = vec3;
    using curveeval = std::pair<vec3, vec3>;

    // triangular index
    // todo : move this to maths namespace
    template<uint n>
    struct triindex
    {
        constexpr triindex(uint _i, uint _j, uint _k) : i(_i), j(_j), k(_k) {}

        constexpr uint to1d() const
        {
            return to1d(this->j, this->k);
        }

        static constexpr uint to1d(uint j, uint k)
        {
            // n - j gives us row index
            return (n - j) * (n - j + 1) / 2 + k;
        }

        static triindex to3d(uint idx1d)
        {
            // determine which row the control point belongs to
            // add a very small tolerance to account for potential FP inaccuracies
            // where is the derivation?
            float const temp = (sqrt((static_cast<float>(idx1d) + 1.f) * 8.f + 1.f) - 1.f) / 2.0f - 0.000001f;

            // Convert to 0 based index
            uint const row = static_cast<uint>(stdx::ceil(temp)) - 1u;

            // This will give the index of starting vertex of the row
            // Number of vertices till layer n(0-based) is n(n+1)/2
            uint const rowStartVertex = (row) * (row + 1) / 2;

            uint const j = n - row;
            uint const k = idx1d - rowStartVertex;
            uint const i = n - j - k;

            return triindex(i, j, k);
        }

        uint i, j, k;
    };

    // n is dimension of the hypercube(0 based)
    template<uint n>
    requires (n >= 0)
    struct hypercubeidx
    {
        constexpr hypercubeidx() = default;
        constexpr hypercubeidx(std::array<uint, n + 1> const& _coords) : coords(_coords) {}
        constexpr uint operator[](uint idxidx) const { return coords[idxidx]; }
        constexpr hypercubeidx operator+(hypercubeidx const& rhs) const { return { stdx::sum(coords, rhs.coords) }; }

        // d is degree(0 based)
        template<uint d>
        static constexpr uint to1d(hypercubeidx const& idx) 
        { 
            uint res = 0;
            // idx = x0 + x1d + x2dd + x3ddd + ....
            for (uint i(n); i >= 1; --i)
                res = (idx.coords[i] + res) * (d + 1);

            res += idx.coords[0];
            return res;
        }

        template<uint d>
        static constexpr hypercubeidx from1d(uint idx)
        {
            hypercubeidx res;
            for (auto i : std::ranges::iota_view{ 0u,  n + 1 })
                res.coords[i] = (idx / stdx::pown((d + 1), i)) % (d + 1);

            return res;
        }

        // specialization to avoid pown(0,0)
        template<>
        static constexpr hypercubeidx from1d<0>(uint idx) 
        { 
            hypercubeidx res;
            std::ranges::fill(res.coords, 0u);
            return res;
        }

        std::array<uint, n + 1> coords;
    };

    template<uint n>
    struct beziercurve
    {
        constexpr beziercurve() = default;
        constexpr vec3& operator[](uint index) { return controlnet[index]; }
        constexpr vec3 const& operator[](uint index) const { return controlnet[index]; }
        static constexpr uint numcontrolpts = n + 1;
        std::array<controlpoint, numcontrolpts> controlnet;
    };

    template<uint n>
    struct beziersurface
    {
        constexpr beziersurface() = default;
        constexpr vec3& operator[](uint index) { return controlnet[index]; }
        constexpr vec3 operator[](uint index) const { return controlnet[index]; }
        static constexpr uint numcontrolpts = (n + 1) * (n + 1);
        std::array<controlpoint, numcontrolpts> controlnet;
    };

    template<uint n>
    struct beziertriangle
    {
        constexpr beziertriangle() = default;
        constexpr vec3& operator[](uint index) { return controlnet[index]; }
        constexpr vec3 operator[](uint index) const { return controlnet[index]; }
        static constexpr uint numcontrolpts = ((n + 1) * (n + 2)) / 2;
        std::array<controlpoint, numcontrolpts> controlnet;
    };

    template<uint n>
    struct beziervolume
    {
        constexpr beziervolume() = default;
        constexpr vec3& operator[](uint index) { return controlnet[index]; }
        constexpr vec3 operator[](uint index) const { return controlnet[index]; }
        static constexpr uint numcontrolpts = (n + 1) * (n + 1) * (n + 1);
        std::array<controlpoint, numcontrolpts> controlnet;
    };

    template<uint n, uint m>
    struct beziershape
    {
        beziertriangle<n> Patches[m] = {};
        static constexpr uint gdgree() { return n; }
        static constexpr uint gnumpatches() { return m; }
        vec3 gcenter() const { return vec3::Zero; }
    };

    template<uint n, uint s>
    requires(n >= 0 && n >= s)  // degree cannot be negative or smaller than subdivision end degree
    struct decasteljau
    {
        static beziercurve<s> curve(beziercurve<n> const& curve, float t)
        {
            beziercurve<n - 1u> subcurve;
            for (uint i = 0; i < subcurve.numcontrolpts; ++i)
                // linear interpolation to calculate subcurve controlnet
                subcurve.controlnet[i] = curve.controlnet[i] * (1 - t) + curve.controlnet[i + 1] * t;

            return decasteljau<n - 1u, s>::curve(subcurve, t);
        }

        static beziertriangle<s> triangle(beziertriangle<n> const& patch, vec3 const& uvw)
        {
            beziertriangle<n - 1u> subpatch;
            for (uint i = 0; i < subpatch.numcontrolpts; ++i)
            {
                // barycentric interpolation to calculate subpatch controlnet
                auto const& idx = triindex<n - 1u>::to3d(i);
                subpatch.controlnet[i] = patch[triindex<n>::to1d(idx.j, idx.k)] * uvw.x + patch[triindex<n>::to1d(idx.j + 1, idx.k)] * uvw.y + patch[triindex<n>::to1d(idx.j, idx.k + 1)] * uvw.z;
            }

            return decasteljau<n - 1u, s>::triangle(subpatch, uvw);
        }

        static beziersurface<s> surface(beziersurface<n> const& patch, vec2 const& uv)
        {
            static constexpr hypercubeidx<1> u0{ { 1, 0 } };
            static constexpr hypercubeidx<1> u1{ { 0, 1 } };

            auto const [u, v] = uv;
            beziersurface<n - 1u> subpatch;
            for (uint i = 0; i < subpatch.numcontrolpts; ++i)
            {
                // bilinear interpolation to calculate subpatch controlnet
                auto const& idx = hypercubeidx<1>::from1d<n - 1u>(i);
                subpatch.controlnet[i] = (patch[hypercubeidx<1>::to1d<n>(idx)] * (1.f - u) + patch[hypercubeidx<1>::to1d<n>(idx) + u0] * u) * (1.f - v)
                    + (patch[hypercubeidx<1>::to1d<n>(idx + u1)] * (1.f - u) + patch[hypercubeidx<1>::to1d<n>(idx) + u0 + u1] * u) * v;
            }

            return decasteljau<n - 1u, s>::surface(subpatch, uv);
        }

        static beziervolume<s> volume(beziervolume<n> const& vol, vec3 const& uvw)
        {
            static constexpr hypercubeidx<2> u0({1, 0, 0});
            static constexpr hypercubeidx<2> u1({0, 1, 0});
            static constexpr hypercubeidx<2> u2({0, 0, 1});

            auto const [t0, t2, t1] = uvw;
            beziervolume<n - 1u> subvol;
            for (uint i = 0; i < subvol.numcontrolpts; ++i)
            {   
                auto const& idx = hypercubeidx<2>::from1d<n - 1u>(i);

                // trilinear intepolation to calculate subvol controlnet
                // 100 is the first edge(edge with end point at 100) along x(first dimension)
                // 8 cube verts from 000 to 111
                vec3 const c100 = vol[hypercubeidx<2>::to1d<n>(idx)] * (1.f - t0) + vol[hypercubeidx<2>::to1d<n>(idx + u0)] * t0;
                vec3 const c110 = vol[hypercubeidx<2>::to1d<n>(idx + u1)] * (1.f - t0) + vol[hypercubeidx<2>::to1d<n>(idx + u0 + u1)] * t0;
                vec3 const c101 = vol[hypercubeidx<2>::to1d<n>(idx + u2)] * (1.f - t0) + vol[hypercubeidx<2>::to1d<n>(idx + u0 + u2)] * t0;
                vec3 const c111 = vol[hypercubeidx<2>::to1d<n>(idx + u1 + u2)] * (1.f - t0) + vol[hypercubeidx<2>::to1d<n>(idx + u0 + u1 + u2)] * t0;
                
                subvol.controlnet[i] = (c100 * (1 - t1) + c110 * t1) * (1 - t2) + (c101 * (1 - t1) + c111 * t1) * t2;
            }

            return decasteljau<n - 1u, s>::volume(subvol, uvw);
        }
    };

    template<uint n>
    requires (n >= 0)
    struct decasteljau<n, n>
    {
        static constexpr beziercurve<n> curve(beziercurve<n> const& curve, float t) { return curve; }
        static constexpr beziertriangle<n> triangle(beziertriangle<n> const& patch, vec3 const& uvw) { return patch; }
        static constexpr beziersurface<n> surface(beziersurface<n> const& patch, vec2 const& uv) { return patch; }
        static constexpr beziervolume<n> volume(beziervolume<n> const& vol, vec3 const& uvw) { return vol; }
    };

    // evaluate position and tangent
    template<uint n>
    requires (n > 1)
    static constexpr curveeval evaluate(beziercurve<n> const& curve, float t)
    {
        auto const& line = decasteljau<n, 1>::curve(curve, t);
        return { line.controlnet[0] * (1 - t) + line.controlnet[1] * t, (line.controlnet[1] - line.controlnet[0]).Normalized() };
    };

    static constexpr vec3 evaluate(beziercurve<1> const& curve, float t) { return decasteljau<1, 0>::curve(curve, t).controlnet[0]; }; 

    template<uint n>
    static constexpr geometry::vertex evaluate(beziertriangle<n> const& patch, vec3 const& uvw)
    {
        auto const& triangle = decasteljau<n, 1>::triangle(patch, uvw);
        controlpoint const& p010 = triangle[triindex<n>::to1d(1, 0)];
        controlpoint const& p100 = triangle[triindex<n>::to1d(0, 0)];
        controlpoint const& p001 = triangle[triindex<n>::to1d(0, 1)];

        return { { p100 * uvw.x + p010 * uvw.y + p001 * uvw.z }, (p100 - p010).Cross(p001 - p010).Normalized() };
    }

    template<uint n>
    static constexpr geometry::vertex evaluate(beziersurface<n> const& vol, vec2 const& uv) 
    {
        auto const &square = decasteljau<n, 1>::surface(vol, uv);
        controlpoint const& p00 = square[hypercubeidx<1>::to1d<1>({ 0,0 })];
        controlpoint const& p10 = square[hypercubeidx<1>::to1d<1>({ 1,0 })];
        controlpoint const& p01 = square[hypercubeidx<1>::to1d<1>({ 0,1 })];

        return { decasteljau<1, 0>::surface(square, uv).controlnet[0], (p01 - p00).Cross(p10 - p00).Normalized() };
    };

    template<uint n>
    requires(n >= 0)
    static constexpr vec3 evaluate(beziervolume<n> const& vol, vec3 const& uwv) {  return decasteljau<n, 0>::volume(vol, uwv).controlnet[0]; };

    template<uint n>
    requires(n >= 0)
    static constexpr vec3 evaluatefast(beziervolume<n> const& vol, vec3 const& uwv) { return decasteljau<n, 0>::volume(vol, uwv).controlnet[0]; };

    static vec3 evaluatefast(beziervolume<2> const& v, vec3 const& uwv) 
    { 
        auto qbasis = [](float t) constexpr
        {
            // quadratic bezier basis
            float const invt = 1.f - t;
            return vec3( invt * invt, 2.f * invt * t, t * t );
        };

        auto const [t0, t2, t1] = uwv;
        vec3 const bt0 = qbasis(t0);
        vec3 const bt1 = qbasis(t1);
        vec3 const bt2 = qbasis(t2);

        vec3 const result = bt2.x * (bt1.x * (v[0] * bt0.x + v[1] * bt0.y + v[2] * bt0.z) + bt1.y * (v[3] * bt0.x + v[4] * bt0.y + v[5] * bt0.z) + bt1.z * (v[6] * bt0.x + v[7] * bt0.y + v[8] * bt0.z)) 
        + bt2.y * (bt1.x * (v[9] * bt0.x + v[10] * bt0.y + v[11] * bt0.z) + bt1.y * (v[12] * bt0.x + v[13] * bt0.y + v[14] * bt0.z) + bt1.z * (v[15] * bt0.x + v[16] * bt0.y + v[17] * bt0.z))
        + bt2.z * (bt1.x * (v[18] * bt0.x + v[19] * bt0.y + v[20] * bt0.z) + bt1.y * (v[21] * bt0.x + v[22] * bt0.y + v[23] * bt0.z) + bt1.z * (v[24] * bt0.x + v[25] * bt0.y + v[26] * bt0.z));

        return result;
    };

    template<uint n>
    constexpr beziertriangle<n + 1> elevate(beziertriangle<n> const& patch)
    {
        beziertriangle<n + 1> elevatedPatch;
        for (int i = 0; i < elevatedPatch.numcontrolpts; ++i)
        {
            auto const& idx = triindex<n + 1>::to3d(i);

            // subtraction will yield negative indices at times ignore such points
            auto const& term0 = idx.i == 0 ? vec3::Zero : patch.controlnet[triindex<n>::to1d(idx.j, idx.k)] * idx.i;
            auto const& term1 = idx.j == 0 ? vec3::Zero : patch.controlnet[triindex<n>::to1d(idx.j - 1, idx.k)] * idx.j;
            auto const& term2 = idx.k == 0 ? vec3::Zero : patch.controlnet[triindex<n>::to1d(idx.j, idx.k - 1)] * idx.k;

            elevatedPatch.controlnet[i] = (term0 + term1 + term2) / (n + 1);
        }

        return elevatedPatch;
    }
    
    template<uint n>
    std::vector<curveeval> tessellate(beziercurve<n> const& curve)
    {
        // 10 divisions per unit length
        static constexpr float stepperunit = 0.1f;
        float curvelen = 0.f;

        // approximate curve length
        for (uint i = 1; i < curve.numcontrolpts; ++i)
            curvelen += vec3::Distance(curve.controlnet[i - 1], curve.controlnet[i]);

        float const step = stepperunit / curvelen;

        std::vector<curveeval> result;
        result.reserve(static_cast<uint>(curvelen / stepperunit));
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
    std::vector<geometry::vertex> tessellate(beziertriangle<n> const& patch)
    {
        static constexpr int numRows = 16;
        static constexpr int numTotalTris = numRows * numRows;
        static constexpr float step = 1.f / numRows;

        std::vector<geometry::vertex> result;

        // reserve space for all triangles
        // use sum of arithemetic series to detrmine the total size
        // turns out the sum of series = n*n
        result.reserve(numTotalTris * 3);

        // add a minor tolerance to account for FP inaccuracies
        float end = 0.f + step - 0.0001f;
        for (int row = 0; row < numRows; ++row)
        {
            float topV, botV;
            topV = 1.f - row * step;
            botV = 1.f - ((row + 1) * step);
            vec3 topLeft = { 1.f - topV, topV, 0.f };
            vec3 topRight = { 0.f, topV, 1.f - topV };

            vec3 botLeft = { 1.f - botV, botV, 0.f };
            vec3 botRight = { 0.f, botV, 1.f - botV };

            // compute the first edge
            vec3 topVert = stdx::lerp(topLeft, topRight, 0.f);
            vec3 botLeftVert = stdx::lerp(botLeft, botRight, 0.f);

            auto lastEdge = std::make_pair<geometry::vertex>(evaluate(patch, botLeftVert), evaluate(patch, topVert));
            int const numRowTris = (2 * row + 1);

            // compute the triangle strip for each layer
            for (int triIdx = 0; triIdx < numRowTris; ++triIdx)
            {
                static const auto norm = vec3{ 0.f, 0.f, 1.f };
                if (triIdx % 2 == 0)
                {
                    // skip intermediate triangles when calculating interpolation t which do not contribute in change of horizontal(uw) parameter span
                    // in total there will be floor(nTris / 2) such triangles
                    vec3 const botRightVert = stdx::lerp(botLeft, botRight, (float(triIdx - (triIdx / 2)) + 1.f) / (float(row) + 1.f));
                    result.push_back(lastEdge.first);
                    result.push_back(lastEdge.second);
                    result.push_back(evaluate(patch, botRightVert));
                    lastEdge = { lastEdge.second, result.back().vertices[2] };
                }
                else
                {
                    // skip intermediate triangles when calculating interpolation t which do not contribute in change of horizontal(uw) parameter span
                    // in total there will be floor(nTris / 2) such triangles
                    vec3 const topRightVert = stdx::lerp(topLeft, topRight, (float(triIdx - (triIdx / 2))) / float(row));
                    result.push_back(lastEdge.first);
                    result.push_back(evaluate(patch, topRightVert));
                    result.push_back(lastEdge.second);
                    lastEdge = { lastEdge.second, result.back().vertices[1] };
                }
            }
        }

        return result;
    }

    template<uint n, uint M>
    std::vector<geometry::vertex> tessellateshape(beziershape<n, M> const& shape)
    {
        std::vector<geometry::vertex> result;
        for (int i = 0; i < M; ++i)
        {
            auto tesselatedpatch = tessellate(shape.Patches[i]);
            std::move(tesselatedpatch.begin(), tesselatedpatch.end(), back_inserter(result));
        }

        return result;
    }

    template<uint n>
    std::vector<vec3> getwireframe_controlmesh(beziertriangle<n> const& patch)
    {
        std::vector<vec3> result;

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
            for (int nextRowPt = previousRowEnd + 1; nextRowPt < previousRowEnd + 1 + row + 1 ; ++nextRowPt)
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
}