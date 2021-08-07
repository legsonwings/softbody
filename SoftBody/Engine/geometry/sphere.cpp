#include "stdafx.h"
#include "sphere.h"
#include "geoutils.h"
#include "triangle.h"

#include <algorithm>

using namespace DirectX;
using namespace geometry;

std::vector<vertex> const& sphere::get_triangles() const
{
    return triangulated_sphere;
}

std::vector<geometry::sphere::polar_coords> const& geometry::sphere::get_triangles_polar() const
{
    return triangulated_sphere_polar;
}

std::vector<linesegment> geometry::sphere::intersect(sphere const& r) const
{
    return intersect(*this, r);
}

void geometry::sphere::sort_tris()
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

void sphere::addquad(float theta, float phi, float step_theta, float step_phi)
{
    // Create the quad(two triangles) using the vertices
    vertex lbv, ltv, rbv, rtv;
    ltv = create_spherevertex(phi, theta);
    rtv = create_spherevertex(phi + step_phi, theta);
    lbv = create_spherevertex(phi, theta + step_theta);
    rbv = create_spherevertex(phi + step_phi, theta + step_theta);

    if (!geoutils::nearlyequal(lbv.position, ltv.position) && !geoutils::nearlyequal(lbv.position, rtv.position) && !geoutils::nearlyequal(ltv.position, rtv.position))
    {
        triangulated_sphere.push_back(lbv);
        triangulated_sphere.push_back(ltv);
        triangulated_sphere.push_back(rtv);
        triangulated_sphere_polar.push_back({ theta + step_theta, phi });
        triangulated_sphere_polar.push_back({ theta, phi });
        triangulated_sphere_polar.push_back({ theta, phi + step_phi });
    }

    if (!geoutils::nearlyequal(lbv.position, rtv.position) && !geoutils::nearlyequal(lbv.position, rbv.position) && !geoutils::nearlyequal(rtv.position, rbv.position))
    {
        triangulated_sphere.push_back(lbv);
        triangulated_sphere.push_back(rtv);
        triangulated_sphere.push_back(rbv);
        triangulated_sphere_polar.push_back({ theta + step_theta, phi });
        triangulated_sphere_polar.push_back({ theta, phi + step_phi });
        triangulated_sphere_polar.push_back({ theta + step_theta, phi + step_phi});
    }
}

void sphere::generate_triangles()
{
    float const step_radians = DirectX::XMConvertToRadians(step_degrees);
    unsigned const num_vertices = static_cast<unsigned>((XM_PI / step_radians) * (XM_2PI / step_radians));

    triangulated_sphere.clear();
    triangulated_sphere.reserve(num_vertices);

    float theta = 0.f, phi = 0.f;

    float reciprocal_stepradians = 1 / step_radians;

    float const theta_end = XM_PI - step_radians + geoutils::tolerance<float>;
    float const phi_end = XM_2PI - step_radians + geoutils::tolerance<float>;
    float const step_theta = XM_PI - std::floor(XM_PI * reciprocal_stepradians) * step_radians;
    float const step_phi = XM_2PI - std::floor(XM_2PI * reciprocal_stepradians) * step_radians;

    // Divide the sphere into quads for each region defined by four circles(at theta, theta + step, phi and phi + step)
    for (theta = 0.f; theta < theta_end; theta += step_radians)
    {
        for (phi = 0.f; phi < phi_end; phi += step_radians)
        {
            addquad(theta, phi, step_radians, step_radians);
        }

        if (step_phi > geoutils::tolerance<>)
        {
            // what are you doing step phi?
            // step phi is not a factor of 2PI, so make the last step smaller
            addquad(theta, phi, step_radians, step_phi);
        }
    }

    if (step_theta > geoutils::tolerance<>)
    {
        for (phi = 0.f; phi < phi_end; phi += step_radians)
        {
            addquad(theta, phi, step_theta, step_radians);
        }

        if (step_phi > geoutils::tolerance<>)
        {
            addquad(theta, phi, step_theta, step_phi);
        }
    }

    sort_tris();
}

vertex sphere::create_spherevertex(float const phi, float const theta)
{
    vertex res;
    res.position = { sinf(theta) * cosf(phi), cosf(theta), sinf(theta) * sinf(phi) };
    res.normal = res.position;
    res.position *= radius;
    res.position += position;
    return res;
}

std::vector<linesegment> sphere::intersect(sphere const& l, sphere const& r)
{
    auto const& lsphereverts = l.get_triangles();
    auto const& rsphereverts = r.get_triangles();

    auto const lnum_verts = lsphereverts.size();
    auto const rnum_verts = rsphereverts.size();

    assert(lnum_verts % 3 == 0);
    assert(rnum_verts % 3 == 0);
    
    std::vector<triangle> ltriangles;
    ltriangles.reserve(lnum_verts % 3);
    for (uint i = 0; i < lnum_verts; i += 3)
    {
        ltriangles.emplace_back(triangle{ lsphereverts[i].position, lsphereverts[i + 1].position, lsphereverts[i + 2].position });
    }

    std::vector<triangle> rtriangles;
    ltriangles.reserve(rnum_verts % 3);
    for (uint i = 0; i < rnum_verts; i += 3)
    {
        rtriangles.emplace_back(triangle{ rsphereverts[i].position, rsphereverts[i + 1].position, rsphereverts[i + 2].position });
    }

    std::vector<linesegment> result;
    for(auto const &ltri : ltriangles)
        for (auto const& rtri : rtriangles)
        {
            if (auto const& isect = triangle::intersect(ltri, rtri))
            {
                result.push_back(isect.value());
            }
        }

    return result;
}