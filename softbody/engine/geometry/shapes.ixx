module;

#include "geocore.h"
#include "geoutils.h"
#include "stdx/stdx.h"
#include "stdx/vec.h"
#include "engine/graphics/gfxcore.h"
#include "engine/graphics/globalresources.h"

#include <utility>
#include <vector>
#include <concepts>
#include <algorithm>
#include <unordered_map>

export module shapes;

export namespace geometry
{
struct rectangle
{
    std::vector<geometry::vertex> triangles() const
    {
        float const halfl = _dims[0] / 2.f;
        float const halfh = _dims[1] / 2.f;
        vector3 const normal = vector3::Forward;
        std::vector<geometry::vertex> r;
        r.push_back({ _center + vector3{-halfl, halfh, 0.f}, normal, vector2{0.f, 0.f} });
        r.push_back({ _center + vector3{halfl, halfh, 0.f}, normal, vector2{1.f, 0.f} });
        r.push_back({ _center + vector3{-halfl, -halfh, 0.f}, normal, vector2{0.f, 1.f} });
        r.push_back({ _center + vector3{-halfl, -halfh, 0.f}, normal, vector2{0.f, 1.f} });
        r.push_back({ _center + vector3{halfl, halfh, 0.f}, normal, vector2{1.f, 0.f} });
        r.push_back({ _center + vector3{halfl, -halfh, 0.f}, normal, vector2{1.f, 1.f} });
        return r;
    }

    stdx::vec2 _dims = {};
    vector3 _center = {};
};

struct circle
{
    std::vector<vertex> triangles() const
    {
        std::vector<vector3> vertices;

        float x0 = radius * std::cosf(0);
        float y0 = radius * std::sinf(0);

        for (float t = step; t <= (XM_2PI + step); t += step)
        {
            float const x = radius * std::cosf(t);
            float const y = radius * std::sinf(t);

            vertices.push_back(vector3::Zero);
            vertices.push_back({ x, y , 0.f });
            vertices.push_back({ x0, y0 , 0.f });

            x0 = x;
            y0 = y;
        }

        matrix plane_tx = geoutils::get_planematrix(center, normal);

        std::vector<vertex> result;
        for (auto& vert : vertices) { result.push_back({ vector3::Transform(vert, plane_tx), normal }); }

        return result;
    }

    float radius = 1.f;
    vector3 center = {};
    vector3 normal = {};

    static constexpr uint numsegments = 20;
    static constexpr float step = XM_2PI / numsegments; // needs to be tested
};

struct cube
{
    cube() = default;
    constexpr cube(vector3 const& _center, vector3 const& _extents) : center(_center), extents(_extents) {}

    aabb const& bbox() const
    {
        auto createaabb = [](auto const& verts)
        {
            std::vector<vector3> positions;
            for (auto const& v : verts) { positions.push_back(v.position); }
            return aabb(positions);
        };

        static const aabb box = createaabb(vertices());
        return box;
    }

    std::vector<geometry::vertex> vertices() const
    {
        static const std::vector<vertex> verts = geoutils::create_cube(center, extents);
        return verts;
    }

    std::vector<geometry::vertex> vertices_flipped() const
    {
        auto invert = [](auto const& verts)
        {
            std::vector<vertex> result;
            result.reserve(verts.size());

            // just flip the position to turn geometry inside out
            for (auto const& v : verts) { result.emplace_back(-v.position, v.normal); }

            return result;
        };

        static const std::vector<vertex> invertedvertices = invert(vertices());
        return invertedvertices;
    }

    std::vector<gfx::instance_data> instancedata() const { return { gfx::instance_data(matrix::CreateTranslation(center), gfx::globalresources::get().view(), gfx::globalresources::get().mat(""))}; }

private:
    vector3 center, extents;
};

struct sphere
{
    using polar_coords = std::pair<float, float>;

    sphere() = default;
    sphere(vector3 const& _center, float _radius) : center(_center), radius(_radius) {}

    vector3 gcenter() const { return center; }
    void scenter(vector3 const &_center) { center = _center; }
    std::vector<vertex> const& triangles() const { return triangulated_sphere; }
    std::vector<polar_coords> const& triangles_polar() const { return triangulated_sphere_polar; }

    void generate_triangles()
    {
        if (unitspheres_tessellated[numsegments_longitude].size() == 0)
            sphere::cacheunitsphere(numsegments_longitude, numsegments_latitude);

        generate_triangles(unitspheres_tessellated[numsegments_longitude]);
    }

private:

    void generate_triangles(std::vector<vector3> const& unitsphere_triangles)
    {
        triangulated_sphere.clear();
        triangulated_sphere.reserve(unitsphere_triangles.size());
        for (auto const& v : unitsphere_triangles)
            triangulated_sphere.emplace_back(v * radius + center, v);
    }

    void sort_tris()
    {
        auto const num_tris = triangulated_sphere.size() / 3;
        std::vector<uint> proxy_tris(num_tris);
        std::sort(proxy_tris.begin(), proxy_tris.end(), [tris = triangulated_sphere, polar_tris = triangulated_sphere_polar](uint l, uint r)
            {
                uint const lv = l * 3;
                uint const rv = r * 3;

                float minphi_l, minphi_r, mintheta_l, mintheta_r;
                minphi_l = minphi_r = mintheta_l = mintheta_r = std::numeric_limits<float>::max();

                for (uint i = 0; i < 3; ++i)
                {
                    float const phi_l = polar_tris[lv + i].second;
                    float const phi_r = polar_tris[rv + i].second;
                    float const theta_l = polar_tris[lv + i].first;
                    float const theta_r = polar_tris[rv + i].first;

                    if (phi_l < minphi_l)
                        minphi_l = phi_l;
                    if (phi_r < minphi_r)
                        minphi_r = phi_r;

                    if (theta_l < mintheta_l)
                        mintheta_l = theta_l;
                    if (theta_r < mintheta_r)
                        mintheta_r = theta_r;
                }

                return minphi_l < minphi_r || mintheta_l < mintheta_r;
            });

        auto const temp_tris = triangulated_sphere;
        auto const temp_polars = triangulated_sphere_polar;

        for (std::size_t i = 0; i < num_tris; ++i)
        {
            auto const tri_idx = proxy_tris[i];
            triangulated_sphere[tri_idx] = temp_tris[tri_idx];
            triangulated_sphere[tri_idx + 1] = temp_tris[tri_idx + 1];
            triangulated_sphere[tri_idx + 2] = temp_tris[tri_idx + 2];

            triangulated_sphere_polar[tri_idx] = temp_polars[tri_idx];
            triangulated_sphere_polar[tri_idx + 1] = temp_polars[tri_idx + 1];
            triangulated_sphere_polar[tri_idx + 2] = temp_polars[tri_idx + 2];
        }
    }

    static void cacheunitsphere(uint numsegments_longitude, uint numsegments_latitude)
    {
        float const stepphi = XM_2PI / numsegments_longitude;
        float const steptheta = XM_PI / numsegments_latitude;
        uint const num_vertices = numsegments_longitude * numsegments_latitude * 4;

        auto& unitsphere = unitspheres_tessellated[numsegments_longitude];
        unitsphere.clear();
        unitsphere.reserve(num_vertices);

        float const theta_end = XM_PI - steptheta + stdx::tolerance<float>;
        float const phi_end = XM_2PI - stepphi + stdx::tolerance<float>;

        // Divide the sphere into quads for each region defined by four circles(at theta, theta + step, phi and phi + step)
        for (float theta = 0.f; theta < theta_end; theta += steptheta)
            for (float phi = 0.f; phi < phi_end; phi += stepphi)
            {
                vector3 lbv, ltv, rbv, rtv;
                ltv = spherevertex(phi, theta);
                rtv = spherevertex(phi + stepphi, theta);
                lbv = spherevertex(phi, theta + steptheta);
                rbv = spherevertex(phi + stepphi, theta + steptheta);

                if (!geoutils::nearlyequal(lbv, ltv) && !geoutils::nearlyequal(lbv, rtv) && !geoutils::nearlyequal(ltv, rtv))
                {
                    unitsphere.push_back(lbv);
                    unitsphere.push_back(ltv);
                    unitsphere.push_back(rtv);
                }

                if (!geoutils::nearlyequal(lbv, rtv) && !geoutils::nearlyequal(lbv, rbv) && !geoutils::nearlyequal(rtv, rbv))
                {
                    unitsphere.push_back(lbv);
                    unitsphere.push_back(rtv);
                    unitsphere.push_back(rbv);
                }
            }
    }

    static vector3 spherevertex(float const phi, float const theta) { return { std::sinf(theta) * std::cosf(phi), std::cosf(theta), std::sinf(theta) * std::sinf(phi) }; }

    std::vector<vertex> triangulated_sphere;
    std::vector<polar_coords> triangulated_sphere_polar;
    static inline std::unordered_map<uint, std::vector<vector3>> unitspheres_tessellated;
public:

    float radius = 1.5f;
    vector3 center = {};

    uint numsegments_longitude = 24;
    uint numsegments_latitude = (numsegments_longitude / 2) + (numsegments_longitude % 2);
};

}