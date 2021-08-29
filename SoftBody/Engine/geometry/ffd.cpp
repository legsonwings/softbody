#include "ffd.h"
#include "geoutils.h"
#include "engine/graphics/gfxutils.h"

#include <ranges>
#include <vector>
#include <algorithm>

import spring;
import primitives;

using namespace geometry;
using namespace DirectX;

float fact(uint i)
{
    if (i < 1)
        return 1;

    return fact(i - 1) * i;
}

std::vector<linesegment> intersect(ffd_object const& l, ffd_object const& r)
{
    auto const& isect_box = l.aabbworld().intersect(r.aabbworld());
    if (!isect_box)
        return {};

    auto const& ltris = l.physx_triangles();
    auto const& rtris = r.physx_triangles();

    std::vector<stdx::ext<aabb, uint>> laabbs, raabbs;

    laabbs.reserve(10);
    raabbs.reserve(10);

    auto const lnum_verts = ltris.size();
    auto const rnum_verts = rtris.size();

    assert(lnum_verts % 3 == 0);
    assert(rnum_verts % 3 == 0);

    for (uint lidx = 0; lidx < lnum_verts; lidx += 3)
    {
        stdx::ext<aabb, uint> lbox{ {&ltris[lidx], 3}, lidx };
        if (isect_box.value().intersect(*lbox)) { laabbs.emplace_back(lbox); }
    }

    for (uint ridx = 0; ridx < rnum_verts; ridx += 3)
    {
        stdx::ext<aabb, uint> rbox{ {&rtris[ridx], 3}, ridx };
        if (isect_box.value().intersect(*rbox)) { raabbs.emplace_back(rbox); }
    }

    std::vector<linesegment> result;
    for (uint lidx = 0; lidx < laabbs.size(); lidx++)
        for (uint ridx = 0; ridx < raabbs.size(); ridx++)
            if (laabbs[lidx]->intersect(raabbs[ridx]))
                if (auto const& isect = triangle::intersect(&(ltris[laabbs[lidx].ex()]), &rtris[raabbs[ridx].ex()]))
                    result.push_back(isect.value());

    return result;
}

void ffd_object::update(float dt)
{
    static const physx::spring spring;
    for (uint ctrlpt_idx = 0; ctrlpt_idx < num_control_points; ++ctrlpt_idx)
    {
        auto const displacement = control_points[ctrlpt_idx] - rest_config[ctrlpt_idx];
 
        auto const [delta_pos_ctrlpt, newvel] = spring.critical(displacement, velocities[ctrlpt_idx], dt);
        auto const targetdir = delta_pos_ctrlpt.Normalized();
        
        // clamp the displacement from equilibrium so that control points do not cross the center
        control_points[ctrlpt_idx] = rest_config[ctrlpt_idx] + std::min(delta_pos_ctrlpt.Length(), restsize * 0.99f / 2.f) * targetdir;

        velocities[ctrlpt_idx] = newvel;
    }

    velocity += compute_wholebody_forces() * dt;

    // center is not the geometric center and is not affected by deformations
    auto const delta_pos = velocity * dt;
    center += delta_pos;
    box = aabb{ control_points.data(), control_points.size() };

    physx_verts.clear();
    evaluated_verts.clear();
    for (auto const& vert : vertices)
    {
        auto const pos = evalbez_trivariate(vert.position.x, vert.position.y, vert.position.z);

        // todo : evaluate the normal as well
        evaluated_verts.push_back({ pos, vert.normal });
        physx_verts.push_back(pos + center);
    }
}

std::vector<gfx::instance_data> ffd_object::controlnet_instancedata() const
{
    std::vector<gfx::instance_data> instances_info;
    instances_info.reserve(num_control_points);
    for (auto const& ctrl_pt : control_points) { instances_info.emplace_back(matrix::CreateTranslation(ctrl_pt + center), gfx::getview(), gfx::getmat("")); }
    return instances_info;
}

vec3 ffd_object::evalbez_trivariate(float s, float t, float u) const
{
    auto qbasis = [](float t)
    {
        // quadratic bezier basis
        float const invt = 1.f - t;
        return vec3{ invt * invt, 2.f * invt * t, t * t };
    };

    vec3 const bs = qbasis(s);
    vec3 const bt = qbasis(t);
    vec3 const bu = qbasis(u);

    auto const &v = control_points;

    vec3 const result = bt.x * (bu.x * (v[0] * bs.x + v[1] * bs.y + v[2] * bs.z) + bu.y * (v[3] * bs.x + v[4] * bs.y + v[5] * bs.z) + bu.z * (v[6] * bs.x + v[7] * bs.y + v[8] * bs.z)) 
    + bt.y * (bu.x * (v[9] * bs.x + v[10] * bs.y + v[11] * bs.z) + bu.y * (v[12] * bs.x + v[13] * bs.y + v[14] * bs.z) + bu.z * (v[15] * bs.x + v[16] * bs.y + v[17] * bs.z))
    + bt.z * (bu.x * (v[18] * bs.x + v[19] * bs.y + v[20] * bs.z) + bu.y * (v[21] * bs.x + v[22] * bs.y + v[23] * bs.z) + bu.z * (v[24] * bs.x + v[25] * bs.y + v[26] * bs.z));

    return result;
}

vec3 ffd_object::eval_bez_trivariate(float s, float t, float u) const
{
    // this is just for illustration, of the bernstein polynomial(3d) evaluation
    // much faster evaluation is available(see evalbez_trivariate)
    vec3 result = vec3::Zero;
    for (uint i = 0; i <= 2; ++i)
    {
        vec3 resultj = vec3::Zero;
        float basis_s = (float(l) / float(fact(i) * fact(l - i))) * std::powf(float(1 - s), float(l - i)) * std::powf(s, float(i));
        for (uint j = 0; j <= 2; ++j)
        {
            vec3 resultk = vec3::Zero;
            float basis_t = (float(m) / float(fact(j) * fact(m - j))) * std::powf(float(1 - t), float(m - j)) * std::powf(t, float(j));
            for (uint k = 0; k <= 2; ++k)
            {
                float basis_u = (float(n) / float(fact(k) * fact(n - k))) * std::powf(float(1 - u), float(n - k)) * std::powf(u, float(k));
                resultk += basis_u * control_points[to1D(i, j, k)];
            }

            resultj += resultk * basis_t;
        }

        result += resultj * basis_s;
    }

    return result;
}

vec3 ffd_object::parametric_coordinates(vec3 const& cartesian_coordinates, vec3 const& span)
{
    vec3 const to_point = cartesian_coordinates + span / 2.f;

    // assume the volume is axis aligned and coordinates relative to volume center)
    return { to_point.x / span.x, to_point.y / span.y, to_point.z / span.z };
}

std::vector<vec3> geometry::ffd_object::controlpoint_visualization() const { return geoutils::create_cube_lines(vec3::Zero, 0.1f); }

void ffd_object::move(vec3 delta)
{
    center += delta;
    box.min_pt += delta;
    box.max_pt += delta;
}

vec3 ffd_object::compute_wholebody_forces() const
{
    auto const drag = -velocity * 0.0001f;
    return drag;
}

void ffd_object::resolve_collision(ffd_object & r, float dt)
{
    auto const contacts = compute_contacts(r);
    if (contacts.size() <= 0)
        return;
    
    std::vector<std::pair<uint, uint>> affected_ctrlpts;
    for (uint i = 0; i < contacts.size(); ++i)
    {
        uint const li = closest_controlpoint(contacts[i]);
        uint const ri = r.closest_controlpoint(contacts[i]);
        affected_ctrlpts.push_back({ li, ri });
    }

    static constexpr float body_mass = 1.f;
    static constexpr float ctrlpt_mass = body_mass / num_control_points;

    auto const normal = (center - r.center).Normalized();
    auto const relativevel = velocity - r.velocity;

    static auto constexpr elasticity = 1.f;
    auto const impulse = relativevel.Dot(normal) * elasticity * normal;

    velocity -= impulse;
    r.velocity += impulse;

    // move a bit so they no longer collide, ideally should use mtd
    move(normal * 0.1f);
    r.move(-normal * 0.1f);

    auto const ctrl_impulsemultiplier = 3.5f;
    auto const impulse_per_ctrlpt = impulse * ctrl_impulsemultiplier / static_cast<float>(affected_ctrlpts.size());
    for (uint i = 0; i < affected_ctrlpts.size(); ++i)
    {
        auto const [idx, idx_other] = affected_ctrlpts[i];

        auto const relativev = velocities[idx] - r.velocities[idx_other];
        auto const impulse_ctrlpts = relativev.Dot(normal) * normal * (1.f + elasticity)/ num_control_points;

        velocities[idx] -= (impulse_per_ctrlpt + impulse_ctrlpts);
        r.velocities[idx_other] += (impulse_per_ctrlpt + impulse_ctrlpts);
    }
}

void ffd_object::resolve_collision_interior(aabb const& r, float dt)
{
    // this checks only box-box intersection
    auto const& isect_box = aabbworld().intersect(r);
    if (!isect_box)
        return;

    std::vector<vec3> contacts;

    if (isect_box->max_pt.x >= (r.max_pt.x - geoutils::tolerance<>))
        contacts.emplace_back(isect_box->max_pt.x, center.y, center.z);

    if (isect_box->min_pt.x <= (r.min_pt.x + geoutils::tolerance<>))
        contacts.emplace_back(isect_box->min_pt.x, center.y, center.z);

    if (isect_box->max_pt.y >= (r.max_pt.y - geoutils::tolerance<>))
        contacts.emplace_back(center.x, isect_box->max_pt.y, center.z);

    if (isect_box->min_pt.y <= (r.min_pt.y + geoutils::tolerance<>))
        contacts.emplace_back(center.x, isect_box->min_pt.y, center.z);

    if (isect_box->max_pt.z >= (r.max_pt.z - geoutils::tolerance<>))
        contacts.emplace_back(center.x, center.y, isect_box->max_pt.z);

    if (isect_box->min_pt.z <= (r.min_pt.z + geoutils::tolerance<>))
        contacts.emplace_back(center.x, center.y, isect_box->min_pt.z);

    if (contacts.size() <= 0) return;

    vec3 normal = vec3::Zero;
    std::vector<uint> affected_ctrlpts;
    for (auto const& c : contacts)
    {
        normal += c;
        affected_ctrlpts.push_back(closest_controlpoint(c));
    }

    // average the contacts to find the normal for computing new velocity
    normal /= static_cast<float>(contacts.size());
    normal = (center - normal).Normalized();

    static constexpr float body_mass = 1.f;
    static constexpr float ctrlpt_mass = body_mass / num_control_points;

    static auto constexpr elasticity = 1.f;
    auto const impulse_magnitude = velocity.Dot(-normal) * elasticity;
 
    // reflect along normal
    velocity = velocity.Length() * normal;

    // move it away from wall, to avoid duplicate collisions, ideally should use mtd
    move(normal * 0.1f);

    auto const impulse_per_ctrlpt = 2.f * impulse_magnitude / static_cast<float>(affected_ctrlpts.size());

   //  push control points in
    for (uint i = 0; i < affected_ctrlpts.size(); ++i)
    {
        auto const deltavel_dir = (center - control_points[affected_ctrlpts[i]]).Normalized();
        velocities[affected_ctrlpts[i]] += impulse_per_ctrlpt * deltavel_dir;
    }
}

uint ffd_object::closest_controlpoint(vec3 point) const
{
    point -= center;
    uint ctrlpt_idx = geoutils::invalid<uint>();
    float distsqr_min = std::numeric_limits<float>::max();
    for (uint i = 0; i < num_control_points; ++i)
    {
        if (auto const distsqr = vec3::DistanceSquared(point, control_points[i]); distsqr < distsqr_min)
        {
            distsqr_min = distsqr;
            ctrlpt_idx = i;
        }
    }

    return ctrlpt_idx;
}

std::vector<vec3> ffd_object::compute_contacts(ffd_object const& other) const
{
    auto const lines = intersect(*this, other);

    std::vector<vec3> mids;
    mids.reserve(lines.size() / 2);
    for (uint i = 0; i < lines.size(); i++) { mids.push_back((lines[i].v0 + lines[i].v1) / 2.f); }

    auto constexpr contact_radius = 0.1f;
    auto constexpr contact_radius_squared2 = 2 * contact_radius * contact_radius;
    std::vector<vec3> contacts;

    // naively cluster points around first point that is not in existing clusters
    for (uint i = 0; i < mids.size(); ++i)
    {
        contacts.push_back(mids[i]);
        for (uint j = i + 1; j < mids.size(); ++j)
        {
            if (vec3::DistanceSquared(mids[i], mids[j]) < contact_radius_squared2)
            {
                std::swap(mids[j], mids[mids.size() - 1]);
                mids.pop_back();
                j--;
            }
        }
    }

    return contacts;
}

vec3 ffd_object::compute_contact(ffd_object const& r) const
{
    // compute a simple single contact
    auto const& isect_box = box.intersect(r.gaabb());
    if (!isect_box)
        return geoutils::invalid<vec3>();

    return (isect_box->max_pt + isect_box->min_pt) / 2.f;
}