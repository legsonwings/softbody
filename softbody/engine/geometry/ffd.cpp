#include "ffd.h"
#include "geoutils.h"
#include "engine/graphics/globalresources.h"

#include <ranges>
#include <vector>
#include <algorithm>

import spring;
import primitives;

using namespace geometry;
using namespace DirectX;

geometry::ffd_object::ffd_object(ffddata data) : _center(data.center)
{
    _evaluated_verts = std::move(data.vertices);
    auto const num_verts = _evaluated_verts.size();

    _vertices.reserve(num_verts);
    _physx_verts.reserve(num_verts);

    for (auto const& vert : _evaluated_verts)
        _box += vert.position;

    // correct the geometric center
    const auto boxcenter = _box.center();
    _center += boxcenter;
    for (auto i : std::ranges::iota_view(0u, _evaluated_verts.size()))
    {
        _evaluated_verts[i].position -= boxcenter;
        _physx_verts.emplace_back(_evaluated_verts[i].position + _center);
    }

    auto const& span = _box.span();
    for (auto const& vert : _evaluated_verts)
        _vertices.push_back({ parametric_coordinates(vert.position, span), vert.normal });

    for (uint idx = 0; idx < _volume.numcontrolpts; ++idx)
    {
        // calculate in range [-span/2, span/2]
        using cubeidx = stdx::grididx<2>;
        static constexpr float subtt = dim * 0.5f;
        auto const idx3d = cubeidx::from1d(dim, idx);
        _rest_config[idx] = vector3{ span.x * (idx3d[0] - subtt), span.y * (idx3d[2] - subtt), span.z * (idx3d[1] - subtt) } / static_cast<float>(dim);
        _volume.controlnet[idx] = _rest_config[idx];
    }

    _restsize = span.Length();
}

std::vector<linesegment> intersect(ffd_object const& l, ffd_object const& r)
{
    auto const& isect_box = l.bboxworld().intersect(r.bboxworld());
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
    static const physx::spring spring{};
    for (uint ctrlpt_idx = 0; ctrlpt_idx < _volume.numcontrolpts; ++ctrlpt_idx)
    {
        auto const displacement = _volume.controlnet[ctrlpt_idx] - _rest_config[ctrlpt_idx];
 
        auto const [deltapos_ctrlpt, newvel] = spring.damped(displacement, _velocities[ctrlpt_idx], dt);
        auto const targetdir = deltapos_ctrlpt.Normalized();
        
        // clamp the displacement from equilibrium so that control points do not cross the center(some objects will escape boxes otherwise)
        _volume.controlnet[ctrlpt_idx] = _rest_config[ctrlpt_idx] + std::min(deltapos_ctrlpt.Length(), _restsize * 0.96f / 2.f) * targetdir;

        _velocities[ctrlpt_idx] = newvel;
    }

    _velocity += compute_wholebodyforces() * dt;

    // center is not the geometric center and is not affected by deformations
    auto const delta_pos = _velocity * dt;
    _center += delta_pos;
    _box = aabb{ _volume.controlnet.data(), _volume.controlnet.size() };

    _physx_verts.clear();
    _evaluated_verts = beziermaths::bulkevaluate(_volume, _vertices);

    for (auto const& vtx : _evaluated_verts)
        _physx_verts.emplace_back(vtx.position + _center);
}

std::vector<gfx::instance_data> ffd_object::controlnet_instancedata() const
{
    std::vector<gfx::instance_data> instances_info;
    instances_info.reserve(_volume.numcontrolpts);
    for (auto const& ctrl_pt : _volume.controlnet) { instances_info.emplace_back(matrix::CreateTranslation(ctrl_pt + _center), gfx::globalresources::get().view(), gfx::globalresources::get().mat("")); }
    return instances_info;
}

vector3 ffd_object::eval_bez_trivariate(float s, float t, float u) const
{
    static float (*fact)(uint i) = [](uint i)->float { return i < 1 ? 1 : fact(i - 1) * i; };

    // this is just for illustration, of the bernstein polynomial(3d) evaluation
    // much much much faster evaluation is available
    static constexpr uint l = 2, m = 2, n = 2;
    auto to1d = [](uint i, uint j, uint k) { return i + k * (l + 1) + j * (l + 1) * (m + 1); };
    vector3 result = vector3::Zero;
    for (uint i = 0; i <= 2; ++i)
    {
        vector3 resultj = vector3::Zero;
        float basis_s = (float(l) / float(fact(i) * fact(l - i))) * std::powf(float(1 - s), float(l - i)) * std::powf(s, float(i));
        for (uint j = 0; j <= 2; ++j)
        {
            vector3 resultk = vector3::Zero;
            float basis_t = (float(m) / float(fact(j) * fact(m - j))) * std::powf(float(1 - t), float(m - j)) * std::powf(t, float(j));
            for (uint k = 0; k <= 2; ++k)
            {
                float basis_u = (float(n) / float(fact(k) * fact(n - k))) * std::powf(float(1 - u), float(n - k)) * std::powf(u, float(k));
                resultk += basis_u * _volume.controlnet[to1d(i, j, k)];
            }

            resultj += resultk * basis_t;
        }

        result += resultj * basis_s;
    }

    return result;
}

vector3 ffd_object::parametric_coordinates(vector3 const& cartesian_coordinates, vector3 const& span)
{
    vector3 const to_point = cartesian_coordinates + span / 2.f;

    // assume the volume is axis aligned and coordinates relative to volume center)
    return { to_point.x / span.x, to_point.y / span.y, to_point.z / span.z };
}

std::vector<vector3> geometry::ffd_object::controlpoint_visualization() const { return geoutils::create_cube_lines(vector3::Zero, 0.1f); }

void ffd_object::move(vector3 delta)
{
    _center += delta;
    _box.min_pt += delta;
    _box.max_pt += delta;
}

vector3 ffd_object::compute_wholebodyforces() const
{
    auto const drag = -_velocity * 0.001f;
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
    static constexpr float ctrlpt_mass = body_mass / _volume.numcontrolpts;

    auto const normal = (_center - r._center).Normalized();
    auto const relativevel = _velocity - r._velocity;

    static auto constexpr elasticity = 1.f;
    auto const impulse = relativevel.Dot(normal) * elasticity * normal;

    _velocity -= impulse;
    r._velocity += impulse;

    // move a bit so they no longer collide, ideally should use mtd
    move(normal * 0.1f);
    r.move(-normal * 0.1f);

    auto const ctrl_impulsemultiplier = 3.f;
    auto const impulse_per_ctrlpt = impulse * ctrl_impulsemultiplier / static_cast<float>(affected_ctrlpts.size());
    for (uint i = 0; i < affected_ctrlpts.size(); ++i)
    {
        auto const [idx, idx_other] = affected_ctrlpts[i];

        auto const relativev = _velocities[idx] - r._velocities[idx_other];
        auto const impulse_ctrlpts = relativev.Dot(normal) * normal * (1.f + elasticity)/ static_cast<float>(_volume.numcontrolpts);

        _velocities[idx] -= (impulse_per_ctrlpt + impulse_ctrlpts);
        r._velocities[idx_other] += (impulse_per_ctrlpt + impulse_ctrlpts);
    }
}

void ffd_object::resolve_collision_interior(aabb const& r, float dt)
{
    // this checks only box-box intersection
    auto const& isect_box = bboxworld().intersect(r);
    if (!isect_box)
        return;

    std::vector<vector3> contacts;

    if (isect_box->max_pt.x >= (r.max_pt.x - stdx::tolerance<>))
        contacts.emplace_back(isect_box->max_pt.x, _center.y, _center.z);

    if (isect_box->min_pt.x <= (r.min_pt.x + stdx::tolerance<>))
        contacts.emplace_back(isect_box->min_pt.x, _center.y, _center.z);

    if (isect_box->max_pt.y >= (r.max_pt.y - stdx::tolerance<>))
        contacts.emplace_back(_center.x, isect_box->max_pt.y, _center.z);

    if (isect_box->min_pt.y <= (r.min_pt.y + stdx::tolerance<>))
        contacts.emplace_back(_center.x, isect_box->min_pt.y, _center.z);

    if (isect_box->max_pt.z >= (r.max_pt.z - stdx::tolerance<>))
        contacts.emplace_back(_center.x, _center.y, isect_box->max_pt.z);

    if (isect_box->min_pt.z <= (r.min_pt.z + stdx::tolerance<>))
        contacts.emplace_back(_center.x, _center.y, isect_box->min_pt.z);

    if (contacts.size() <= 0) return;

    vector3 normal = vector3::Zero;
    std::vector<uint> affected_ctrlpts;
    for (auto const& c : contacts)
    {
        normal += c;
        affected_ctrlpts.push_back(closest_controlpoint(c));
    }

    // average the contacts to find the normal for computing new velocity
    normal /= static_cast<float>(contacts.size());
    normal = (_center - normal).Normalized();

    static constexpr float body_mass = 1.f;
    static constexpr float ctrlpt_mass = body_mass / _volume.numcontrolpts;

    static auto constexpr elasticity = 1.f;
    auto const impulse_magnitude = _velocity.Dot(-normal) * elasticity;
 
    // reflect along normal
    _velocity = _velocity.Length() * normal;

    // move it away from wall, to avoid duplicate collisions, ideally should use mtd
    move(normal * 0.1f);

    auto const impulse_per_ctrlpt = 2.f * impulse_magnitude / static_cast<float>(affected_ctrlpts.size());

    // push control points in
    for (uint i = 0; i < affected_ctrlpts.size(); ++i)
    {
        auto const deltavel_dir = (_center - _volume.controlnet[affected_ctrlpts[i]]).Normalized();
        _velocities[affected_ctrlpts[i]] += impulse_per_ctrlpt * deltavel_dir;
    }
}

uint ffd_object::closest_controlpoint(vector3 point) const
{
    point -= _center;
    uint ctrlpt_idx = 0;
    float distsqr_min = std::numeric_limits<float>::max();
    for (uint i = 0; i < _volume.numcontrolpts; ++i)
    {
        if (auto const distsqr = vector3::DistanceSquared(point, _volume.controlnet[i]); distsqr < distsqr_min)
        {
            distsqr_min = distsqr;
            ctrlpt_idx = i;
        }
    }

    return ctrlpt_idx;
}

std::vector<vector3> ffd_object::compute_contacts(ffd_object const& other) const
{
    auto const lines = intersect(*this, other);

    std::vector<vector3> mids;
    mids.reserve(lines.size() / 2);
    for (uint i = 0; i < lines.size(); i++) { mids.push_back((lines[i].v0 + lines[i].v1) / 2.f); }

    auto constexpr contact_radius = 0.1f;
    auto constexpr contact_radius_squared2 = 2 * contact_radius * contact_radius;
    std::vector<vector3> contacts;

    // naively cluster points around first point that is not in existing clusters
    for (uint i = 0; i < mids.size(); ++i)
    {
        contacts.push_back(mids[i]);
        for (uint j = i + 1; j < mids.size(); ++j)
        {
            if (vector3::DistanceSquared(mids[i], mids[j]) < contact_radius_squared2)
            {
                std::swap(mids[j], mids[mids.size() - 1]);
                mids.pop_back();
                j--;
            }
        }
    }

    return contacts;
}

vector3 ffd_object::compute_contact(ffd_object const& r) const
{
    // compute a simple single contact
    auto const& isect_box = _box.intersect(r.bbox());
    if (!isect_box)
        return stdx::invalid<vector3>();

    return (isect_box->max_pt + isect_box->min_pt) / 2.f;
}