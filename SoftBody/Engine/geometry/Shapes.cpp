#include "Shapes.h"
#include "engine/graphics/gfxutils.h"

#include <algorithm>
#include <vector>

import primitives;

using namespace geometry;
using namespace DirectX;

std::vector<linesegment> intersect(ffd_object const& l, ffd_object const& r)
{
    auto const& isect_box = l.getaabb().intersect(r.getaabb());
    if (!isect_box)
        return {};

    auto const& ltris = l.get_physx_triangles();
    auto const& rtris = r.get_physx_triangles();

    std::vector<stdx::ext<aabb, uint>> laabbs, raabbs;

    laabbs.reserve(10);
    raabbs.reserve(10);

    auto const lnum_verts = ltris.size();
    auto const rnum_verts = rtris.size();

    assert(lnum_verts % 3 == 0);
    assert(rnum_verts % 3 == 0);

    for (uint lidx = 0; lidx < lnum_verts; lidx += 3)
    {
        stdx::ext<aabb, uint> lbox{ &ltris[lidx], lidx };
        if (isect_box.value().intersect(*lbox)) { laabbs.emplace_back(lbox); }
    }

    for (uint ridx = 0; ridx < rnum_verts; ridx += 3)
    {
        stdx::ext<aabb, uint> rbox{ &rtris[ridx], ridx };
        if (isect_box.value().intersect(*rbox)) { raabbs.emplace_back(rbox); }
    }

    std::vector<linesegment> result;
    // todo : can optimize this by hashing triangles on the spherical domain
    // may need tiling the sphere with polygons
    for (uint lidx = 0; lidx < laabbs.size(); lidx++)
        for (uint ridx = 0; ridx < raabbs.size(); ridx++)
            if (laabbs[lidx]->intersect(raabbs[ridx]))
                if (auto const& isect = triangle::intersect(&(ltris[laabbs[lidx].ex()]), &rtris[raabbs[ridx].ex()]))
                    result.push_back(isect.value());

    return result;
}

void ffd_object::update(float dt)
{
    velocity += compute_wholebody_forces() * dt;

    auto const delta_pos = velocity * dt;

    // todo : this will not be needed once the computations are not in world space
    // update the box points as they are used in computation below
    box.min_pt += delta_pos;
    box.max_pt += delta_pos;

    for (auto& ctrl_pt : control_points)
        ctrl_pt += delta_pos;

    for (uint ctrl_pt_idx = 0; ctrl_pt_idx < control_points.size(); ++ctrl_pt_idx)
    {
        // eta = damping factor
        // omega = std::sqrt(spring_const / mass);
        auto const eta = 0.5f;
        auto const omega = 2.7f;

        auto const alpha = omega * std::sqrt(1 - eta * eta);
        auto const rest_pt = box.min_pt + rest_config[ctrl_pt_idx];
        auto const displacement = control_points[ctrl_pt_idx] - rest_pt;

        // damped shm
        //auto const delta_pos_ctrlpt = std::powf(2.71828f, -omega * eta * dt) * (displacement * std::cos(alpha * dt) + (velocities[ctrl_pt_idx] + omega * eta * displacement) * std::sin(alpha * dt) / alpha);
        //velocities[ctrl_pt_idx] = -std::pow(2.71828f, -omega * eta * dt) * ((displacement * omega * eta - velocities[ctrl_pt_idx] - omega * eta * displacement) * cos(alpha * dt) +
        //    (displacement * alpha + (velocities[ctrl_pt_idx] + omega * eta * displacement) * omega * eta / alpha) * sin(alpha * dt));

        // critically damped shm
        auto const delta_pos_ctrlpt = ((velocities[ctrl_pt_idx] + displacement * omega) * dt + displacement) * std::powf(2.71828f, -omega * dt);
        velocities[ctrl_pt_idx] = (velocities[ctrl_pt_idx] - (velocities[ctrl_pt_idx] + displacement * omega) * omega * dt) * std::powf(2.71828f, -omega * dt);
        
        auto targetdir = delta_pos_ctrlpt;
        targetdir.Normalize();
        
        // clamp the displacement from equilibrium to less than radius
        control_points[ctrl_pt_idx] = rest_pt + std::min(delta_pos_ctrlpt.Length(), ball.radius * 0.99f) * targetdir;
    }

    box = aabb{ control_points };
    center = (box.min_pt + box.max_pt) / 2.f;

    physx_verts.clear();
    evaluated_verts.clear();
    for (auto const& vert : vertices)
    {
        auto pos = evalbez_trivariate(vert.position.x, vert.position.y, vert.position.z);
        physx_verts.push_back(pos);

        // todo : evaluate the normal as well
        evaluated_verts.push_back({ pos, vert.normal });
    }
}

std::vector<gfx::instance_data> geometry::ffd_object::get_controlnet_instancedata() const
{
    std::vector<gfx::instance_data> instances_info;
    instances_info.reserve(control_points.size());
    for (auto const& ctrl_pt : control_points) { instances_info.emplace_back(matrix::CreateTranslation(ctrl_pt), gfx::getview(), gfx::getmat("")); }
    return instances_info;
}

std::vector<vec3> geometry::ffd_object::getbox_vertices() const
{
    return getbox().get_vertices();
}

vec3 geometry::ffd_object::evalbez_trivariate(float s, float t, float u) const
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

    auto const& v = control_points;

    vec3 result = bt.x * (bu.x * (v[0] * bs.x + v[1] * bs.y + v[2] * bs.z) + bu.y * (v[3] * bs.x + v[4] * bs.y + v[5] * bs.z) + bu.z * (v[6] * bs.x + v[7] * bs.y + v[8] * bs.z)) 
    + bt.y * (bu.x * (v[9] * bs.x + v[10] * bs.y + v[11] * bs.z) + bu.y * (v[12] * bs.x + v[13] * bs.y + v[14] * bs.z) + bu.z * (v[15] * bs.x + v[16] * bs.y + v[17] * bs.z))
    + bt.z * (bu.x * (v[18] * bs.x + v[19] * bs.y + v[20] * bs.z) + bu.y * (v[21] * bs.x + v[22] * bs.y + v[23] * bs.z) + bu.z * (v[24] * bs.x + v[25] * bs.y + v[26] * bs.z));

    return result;
}

vec3 geometry::ffd_object::eval_bez_trivariate(float s, float t, float u) const
{
    vec3 result = vec3::Zero;
    for (uint i = 0; i <= 2; ++i)
    {
        vec3 resultj = vec3::Zero;
        float basis_s = (float(l) / float(fact(i) * fact(l - i))) * std::powf(float(1 - s), float(l - i)) * std::powf(float(s), float(i));
        for (uint j = 0; j <= 2; ++j)
        {
            vec3 resultk = vec3::Zero;
            float basis_t = (float(m) / float(fact(j) * fact(m - j))) * std::powf(float(1 - t), float(m - j)) * std::powf(float(t), float(j));
            for (uint k = 0; k <= 2; ++k)
            {
                float basis_u = (float(n) / float(fact(k) * fact(n - k))) * std::powf(float(1 - u), float(n - k)) * std::powf(float(u), float(k));

                resultk += basis_u * control_points[get1D(i, j, k)];
            }

            resultj += resultk * basis_t;
        }

        result += resultj * basis_s;
    }

    return result;
}

void geometry::ffd_object::move(vec3 delta)
{
    center += delta;
    box.min_pt += delta;
    box.max_pt += delta;

    for (auto& ctrl_pt : control_points)
        ctrl_pt += delta;

    for (auto& vert : evaluated_verts)
    {
        vert.position += delta;
        physx_verts.push_back(vert.position);
    }
}

box geometry::ffd_object::getbox() const
{
    return box;
}

aabb const& geometry::ffd_object::getaabb() const
{
    return box;
}

vec3 geometry::ffd_object::compute_wholebody_forces() const
{
    auto const drag = -velocity * 0.0001f;
    return drag;
}

void geometry::ffd_object::set_velocity(vec3 const &vel)
{
    velocity = vel;
}

void geometry::ffd_object::resolve_collision(ffd_object & r, float dt)
{
    auto const contacts = compute_contacts(r);
    if (contacts.size() <= 0)
    {
        return;
    }
    
    std::vector<std::pair<uint, uint>> affected_ctrlpts;
    for (uint i = 0; i < contacts.size(); ++i)
    {
        uint const li = closest_controlpoint(contacts[i]);
        uint const ri = r.closest_controlpoint(contacts[i]);
        affected_ctrlpts.push_back({ li, ri });
    }

    static constexpr float body_mass = 1.f;
    static constexpr float ctrlpt_mass = body_mass / num_control_points;

    auto normal = center - r.center;
    normal.Normalize();

    auto const pre = velocity + r.velocity;

    auto const relativevel = velocity - r.velocity;

    static auto constexpr elasticity = 1.f;
    auto const impulse = relativevel.Dot(normal) * elasticity * normal;

    // todo : moving control points should exert forces even if whole body is not moving
    // apply a small correction force to account for this for now
    auto const correction_force = normal * 0.f;

    // apply portion of the impulse to controlpoints
    auto const impulse_wholebody = (impulse + correction_force);

    velocity -= impulse_wholebody;
    r.velocity += impulse_wholebody;

    // move a bit so they no longer collide
    move(normal * 0.1f);
    r.move(-normal * 0.1f);

    auto const ctrl_impulsemultiplier = 3.f;
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

void geometry::ffd_object::resolve_collision_interior(aabb const& r, float dt)
{
    // this checks only box-box intersection
    auto const& isect_box = box.intersect(r);
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
    normal = center - normal;
    normal.Normalize();

    static constexpr float body_mass = 1.f;
    static constexpr float ctrlpt_mass = body_mass / num_control_points;

    static auto constexpr elasticity = 1.f;
    auto const impulse_magnitude = velocity.Dot(-normal) * elasticity;
 
    // reflect along normal
    velocity = velocity.Length() * normal;

    // todo : move it away from wall. though may not be enough if dt is too big
    move(normal * 0.1f);

    auto const impulse_per_ctrlpt = 2.f * impulse_magnitude / static_cast<float>(affected_ctrlpts.size());

   //  push control points in
    for (uint i = 0; i < affected_ctrlpts.size(); ++i)
    {
        auto deltavel_dir = center - control_points[affected_ctrlpts[i]];
        deltavel_dir.Normalize();
        velocities[affected_ctrlpts[i]] += impulse_per_ctrlpt * deltavel_dir;
    }
}

uint geometry::ffd_object::closest_controlpoint(vec3 const& point) const
{
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

std::vector<vec3> geometry::ffd_object::compute_contacts(ffd_object const& other) const
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

vec3 geometry::ffd_object::compute_contact(ffd_object const& r) const
{
    auto const& isect_box = box.intersect(r.getaabb());
    if (!isect_box)
        return geoutils::invalid<vec3>();

    return (isect_box->max_pt + isect_box->min_pt) / 2.f;
}

std::vector<vertex> circle::get_triangles() const
{
    std::vector<vec3> vertices;

    float x0 = radius * cosf(0);
    float y0 = radius * sinf(0);

    for (float t = step; t <= (XM_2PI + step); t+=step)
    {
        float x = radius * cosf(t);
        float y = radius * sinf(t);

        vertices.push_back(vec3::Zero);
        vertices.push_back({x, y , 0.f});
        vertices.push_back({x0, y0 , 0.f});

        x0 = x;
        y0 = y;
    }

    matrix plane_tx = geoutils::get_planematrix(center, normal);

    std::vector<vertex> result;
    for (auto& vert : vertices) { result.push_back({ vec3::Transform(vert, plane_tx), normal }); }

    return result;
}

std::vector<gfx::instance_data> circle::get_instance_data() const
{
    return { gfx::instance_data{matrix::CreateTranslation(center), gfx::getview(), gfx::getmat("")}};
}