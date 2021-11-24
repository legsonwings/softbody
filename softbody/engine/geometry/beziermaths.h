#pragma once

#include "geodefines.h"
#include "geocore.h"
#include "geoutils.h"
#include "stdx/stdx.h"
#include "engine/engineutils.h"
#include <array>
#include <vector>
#include <cstdint>
#include <utility>
#include <iterator>
#include <algorithm>

namespace beziermaths
{
using controlpoint = vector3;
using curveeval = std::pair<vector3, vector3>;
using voleval = std::pair<vector3, matrix>;

// quadratic bezier basis
inline constexpr vector3 qbasis(float t)
{
    float const invt = 1.f - t;
    return vector3(invt * invt, 2.f * invt * t, t * t);
}

// quadratic bezier 1st derivative basis
inline constexpr vector3 dqbasis(float t) { return vector3(2 * (t - 1.f), 2.f - 4 * t, 2 * t); };

template<uint n>
struct beziercurve
{
    constexpr beziercurve() = default;
    constexpr vector3& operator[](uint index) { return controlnet[index]; }
    constexpr vector3 const& operator[](uint index) const { return controlnet[index]; }
    static constexpr uint numcontrolpts = n + 1;
    std::array<controlpoint, numcontrolpts> controlnet;
};

template<uint n>
struct beziersurface
{
    constexpr beziersurface() = default;
    constexpr vector3& operator[](uint index) { return controlnet[index]; }
    constexpr vector3 const& operator[](uint index) const { return controlnet[index]; }
    static constexpr uint numcontrolpts = (n + 1) * (n + 1);
    std::array<controlpoint, numcontrolpts> controlnet;
};

template<uint n>
struct beziertriangle
{
    constexpr beziertriangle() = default;
    constexpr vector3& operator[](uint index) { return controlnet[index]; }
    constexpr vector3 const& operator[](uint index) const { return controlnet[index]; }
    static constexpr uint numcontrolpts = ((n + 1) * (n + 2)) / 2;
    std::array<controlpoint, numcontrolpts> controlnet;
};

template<uint n>
struct beziervolume
{
    constexpr beziervolume() = default;
    constexpr vector3& operator[](uint index) { return controlnet[index]; }
    constexpr vector3 const& operator[](uint index) const { return controlnet[index]; }
    static constexpr uint numcontrolpts = (n + 1) * (n + 1) * (n + 1);
    std::array<controlpoint, numcontrolpts> controlnet;
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

    static beziertriangle<s> triangle(beziertriangle<n> const& patch, vector3 const& uvw)
    {
        beziertriangle<n - 1u> subpatch;
        for (uint i = 0; i < subpatch.numcontrolpts; ++i)
        {
            // barycentric interpolation to calculate subpatch controlnet
            auto const& idx = stdx::triindex<n - 1u>::to3d(i);
            subpatch.controlnet[i] = patch[stdx::triindex<n>::to1d(idx.j, idx.k)] * uvw.x + patch[stdx::triindex<n>::to1d(idx.j + 1, idx.k)] * uvw.y + patch[stdx::triindex<n>::to1d(idx.j, idx.k + 1)] * uvw.z;
        }

        return decasteljau<n - 1u, s>::triangle(subpatch, uvw);
    }

    static beziersurface<s> surface(beziersurface<n> const& patch, vector2 const& uv)
    {
        static constexpr stdx::grididx<1> u0(10);
        static constexpr stdx::grididx<1> u1(1);

        auto const [u, v] = uv;
        beziersurface<n - 1u> subpatch;
        for (uint i = 0; i < subpatch.numcontrolpts; ++i)
        {
            // bilinear interpolation to calculate subpatch controlnet
            auto const& idx = stdx::grididx<1>::from1d(n - 1, i);
            subpatch.controlnet[i] = (patch[stdx::grididx<1>::to1d<n>(idx)] * (1.f - u) + patch[stdx::grididx<1>::to1d<n>(idx) + u0] * u) * (1.f - v)
                + (patch[stdx::grididx<1>::to1d<n>(idx + u1)] * (1.f - u) + patch[stdx::grididx<1>::to1d<n>(idx) + u0 + u1] * u) * v;
        }

        return decasteljau<n - 1u, s>::surface(subpatch, uv);
    }

    static beziervolume<s> volume(beziervolume<n> const& vol, vector3 const& uvw)
    {
        using cubeidx = stdx::grididx<2>;
        static constexpr auto u0 = cubeidx(100);
        static constexpr auto u1 = cubeidx(10);
        static constexpr auto u2 = cubeidx(100);

        auto const [t0, t2, t1] = uvw;
        beziervolume<n - 1u> subvol;
        for (uint i = 0; i < subvol.numcontrolpts; ++i)
        {   
            auto const& idx = cubeidx::from1d(n - 1, i);

            // trilinear intepolation to calculate subvol controlnet
            // 100 is the first edge(edge with end point at 100) along x(first dimension)
            // 8 cube verts from 000 to 111
            vector3 const c100 = vol[cubeidx::to1d<n>(idx)] * (1.f - t0) + vol[cubeidx::to1d<n>(idx + u0)] * t0;
            vector3 const c110 = vol[cubeidx::to1d<n>(idx + u1)] * (1.f - t0) + vol[cubeidx::to1d<n>(idx + u0 + u1)] * t0;
            vector3 const c101 = vol[cubeidx::to1d<n>(idx + u2)] * (1.f - t0) + vol[cubeidx::to1d<n>(idx + u0 + u2)] * t0;
            vector3 const c111 = vol[cubeidx::to1d<n>(idx + u1 + u2)] * (1.f - t0) + vol[cubeidx::to1d<n>(idx + u0 + u1 + u2)] * t0;
                
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
    static constexpr beziertriangle<n> triangle(beziertriangle<n> const& patch, vector3 const& uvw) { return patch; }
    static constexpr beziersurface<n> surface(beziersurface<n> const& patch, vector2 const& uv) { return patch; }
    static constexpr beziervolume<n> volume(beziervolume<n> const& vol, vector3 const& uvw) { return vol; }
};

template<uint n>
requires (n >= 0)
constexpr curveeval evaluate(beziercurve<n> const& curve, float t)
{
    auto const& line = decasteljau<n, 1>::curve(curve, t);
    return { line.controlnet[0] * (1 - t) + line.controlnet[1] * t, (line.controlnet[1] - line.controlnet[0]).Normalized() };
};

template<uint n>
constexpr geometry::vertex evaluate(beziertriangle<n> const& patch, vector3 const& uvw)
{
    auto const& triangle = decasteljau<n, 1>::triangle(patch, uvw);
    controlpoint const& p010 = triangle[triindex<n>::to1d(1, 0)];
    controlpoint const& p100 = triangle[triindex<n>::to1d(0, 0)];
    controlpoint const& p001 = triangle[triindex<n>::to1d(0, 1)];

    return { { p100 * uvw.x + p010 * uvw.y + p001 * uvw.z }, (p100 - p010).Cross(p001 - p010).Normalized() };
}

template<uint n>
constexpr geometry::vertex evaluate(beziersurface<n> const& vol, vector2 const& uv) 
{
    auto const &square = decasteljau<n, 1>::surface(vol, uv);
    controlpoint const& p00 = square[stdx::grididx<1>::to1d<1>(0)];
    controlpoint const& p10 = square[stdx::grididx<1>::to1d<1>(10)];
    controlpoint const& p01 = square[stdx::grididx<1>::to1d<1>(1)];

    return { decasteljau<1, 0>::surface(square, uv).controlnet[0], (p01 - p00).Cross(p10 - p00).Normalized() };
};

template<uint n>
requires(n >= 0)
constexpr vector3 evaluate(beziervolume<n> const& vol, vector3 const& uwv) {  return decasteljau<n, 0>::volume(vol, uwv).controlnet[0]; };

template<uint n>
requires(n >= 0)
constexpr vector3 evaluatefast(beziervolume<n> const& vol, vector3 const& uwv) { return decasteljau<n, 0>::volume(vol, uwv).controlnet[0]; };

voleval evaluatefast(beziervolume<2> const& v, vector3 const& uwv);
std::vector<geometry::vertex> bulkevaluate(beziervolume<2> const& v, std::vector<geometry::vertex> const& vertices);

template<uint n>
constexpr beziertriangle<n + 1> elevate(beziertriangle<n> const& patch)
{
    beziertriangle<n + 1> elevatedPatch;
    for (int i = 0; i < elevatedPatch.numcontrolpts; ++i)
    {
        auto const& idx = triindex<n + 1>::to3d(i);
        // subtraction will yield negative indices at times ignore such points
        auto const& term0 = idx.i == 0 ? vector3::Zero : patch.controlnet[triindex<n>::to1d(idx.j, idx.k)] * idx.i;
        auto const& term1 = idx.j == 0 ? vector3::Zero : patch.controlnet[triindex<n>::to1d(idx.j - 1, idx.k)] * idx.j;
        auto const& term2 = idx.k == 0 ? vector3::Zero : patch.controlnet[triindex<n>::to1d(idx.j, idx.k - 1)] * idx.k;
        elevatedPatch.controlnet[i] = (term0 + term1 + term2) / (n + 1);
    }
    return elevatedPatch;
}
}