module;

#include "geocore.h"
#include "geoutils.h"
#include "engine/engineutils.h"
#include "engine/graphics/gfxcore.h"
#include "engine/graphics/gfxutils.h"

#include <utility>
#include <vector>
#include <algorithm>

export module shapes;

export namespace geometry
{
struct cube
{
    cube() = default;
    constexpr cube(vec3 const& _center, vec3 const& _extents) : center(_center), extents(_extents) {}

    aabb const& getaabb() const
    {
        auto createaabb = [](auto const& verts)
        {
            std::vector<vec3> positions;
            for (auto const& v : verts) { positions.push_back(v.position); }
            return aabb(positions);
        };

        static const aabb box = createaabb(get_vertices());
        return box;
    }

    std::vector<geometry::vertex> get_vertices() const
    {
        static const std::vector<vertex> vertices = geoutils::create_cube(center, extents);
        return vertices;
    }

    std::vector<geometry::vertex> get_vertices_flipped() const
    {
        auto invert = [](auto const& verts)
        {
            std::vector<vertex> result;
            result.reserve(verts.size());

            // just flip the position to turn geometry inside out
            for (auto const& v : verts) { result.emplace_back(-v.position, v.normal); }

            return result;
        };

        static const std::vector<vertex> invertedvertices = invert(get_vertices());
        return invertedvertices;
    }

    std::vector<gfx::instance_data> getinstancedata() const
    {
        return { gfx::instance_data(matrix::CreateTranslation(center), gfx::getview(), gfx::getmat("room")) };
    }

private:
    vec3 center, extents;
    std::vector<geometry::vertex> vertices;
};

struct sphere
{
    using polar_coords = std::pair<float, float>;

    sphere() = default;
    sphere(vec3 const& _position, float _radius) : position(_position), radius(_radius) {}

    std::vector<vertex> const& get_triangles() const { return triangulated_sphere; }
    std::vector<polar_coords> const& get_triangles_polar() const { return triangulated_sphere_polar; }

    void generate_triangles()
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

private:
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

    void addquad(float theta, float phi, float step_theta, float step_phi)
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
            triangulated_sphere_polar.push_back({ theta + step_theta, phi + step_phi });
        }
    }

    vertex create_spherevertex(float const phi, float const theta)
    {
        vertex res;
        res.position = { sinf(theta) * cosf(phi), cosf(theta), sinf(theta) * sinf(phi) };
        res.normal = res.position;
        res.position *= radius;
        res.position += position;
        return res;
    }

    std::vector<vertex> triangulated_sphere;
    std::vector<polar_coords> triangulated_sphere_polar;
public:

    // todo : use num segments instead
    float step_degrees = 15.f;
    float radius = 1.5f;
    vec3 position = {};
};
}