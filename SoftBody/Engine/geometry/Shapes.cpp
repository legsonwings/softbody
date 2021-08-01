#include "stdafx.h"
#include "Shapes.h"
#include "line.h"
#include "triangle.h"
#include "geoutils.h"

#include <algorithm>

#ifdef PROFILE_TIMING
#include <chrono>
#endif 

using namespace DirectX;
using namespace geometry;

void ffd_object::update(float dt)
{
    velocity += compute_wholebody_forces() * dt;

    auto const delta_pos = velocity * dt;
    center += delta_pos;

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
        auto delta_pos = std::powf(2.71828f, -omega * eta * dt) * (displacement * std::cos(alpha * dt) + (velocities[ctrl_pt_idx] + omega * eta * displacement) * std::sin(alpha * dt) / alpha);
        velocities[ctrl_pt_idx] = -std::pow(2.71828f, -omega * eta * dt) * ((displacement * omega * eta - velocities[ctrl_pt_idx] - omega * eta * displacement) * cos(alpha * dt) +
            (displacement * alpha + (velocities[ctrl_pt_idx] + omega * eta * displacement) * omega * eta / alpha) * sin(alpha * dt));

        control_points[ctrl_pt_idx] = rest_pt + delta_pos;
    }

    box = std::move(aabb{ control_points });

    physx_verts.clear();
    evaluated_verts.clear();
    for (auto const& vert : vertices)
    {
        auto pos = eval_bez_trivariate(vert.position.x, vert.position.y, vert.position.z);
        physx_verts.push_back(pos);

        // todo : evaluate the normal as well
        evaluated_verts.push_back({ pos, vert.normal });
    }
}

std::vector<vec3> ffd_object::get_control_point_visualization() const
{
    return geoutils::create_cube_lines(vec3::Zero, 0.1f);
}

std::vector<gfx::instance_data> geometry::ffd_object::get_controlnet_instancedata() const
{
    std::vector<gfx::instance_data> instances_info;
    instances_info.reserve(control_points.size());
    for (auto const& ctrl_pt : control_points) { instances_info.emplace_back(matrix::CreateTranslation(ctrl_pt), vec3{ 1.f, 1.f, 1.f }); }
    return instances_info;
}

std::vector<vec3> geometry::ffd_object::getbox_vertices() const
{
    return getbox().get_vertices();
}

box geometry::ffd_object::getbox() const
{
    return box;
}

aabb const& geometry::ffd_object::getboundingbox() const
{
    return box;
}

vec3 geometry::ffd_object::compute_wholebody_forces() const
{
    auto const drag = -velocity * 0.7f;
    return drag;
}

void geometry::ffd_object::set_velocity(vec3 const vel)
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
        if (uint li = closest_controlpoint(contacts[i]), ri = r.closest_controlpoint(contacts[i]); li < num_control_points && ri < num_control_points)
        {
            affected_ctrlpts.push_back({ li, ri });
        }
    }

    static constexpr float body_mass = 1.f;
    static constexpr float ctrlpt_mass = body_mass / num_control_points;

    auto normal = center - r.center;
    normal.Normalize();

    auto const relativevel = velocity - r.velocity;

    static auto constexpr elasticity = 0.5f;
    auto const impulse = relativevel.Dot(normal) * (1.f + elasticity) * normal;
    
    // apply portion of the impulse to controlpoints
    auto const impulse_wholebody = impulse * 0.45f;

    velocity -= impulse_wholebody;
    r.velocity += impulse_wholebody;

    // todo : try distributing the impulse to neighbouring control points as well, slightly reduced
    auto const impulse_per_ctrlpt = (impulse - impulse_wholebody) / static_cast<float>(affected_ctrlpts.size());
    for (uint i = 0; i < affected_ctrlpts.size(); ++i)
    {
        auto const [idx, idx_other] = affected_ctrlpts[i];

        auto const relativev = velocities[idx] - r.velocities[idx_other];
        auto const impulse_ctrlpts = relativev.Dot(normal) * normal * (1 + elasticity)/ num_control_points;

        velocities[idx] -= (impulse_per_ctrlpt + impulse_ctrlpts);
        r.velocities[idx_other] += (impulse_per_ctrlpt + impulse_ctrlpts);
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

std::vector<linesegment> geometry::ffd_object::intersect(ffd_object const& r) const
{
    auto const& isect_box = box.intersect(r.getboundingbox());
    if (!isect_box)
        return {};

    auto const& ltris = get_physx_triangles();
    auto const& rtris = r.get_physx_triangles();

    std::vector<ext<aabb, uint>> laabbs, raabbs;

    laabbs.reserve(10);
    raabbs.reserve(10);

    auto const lnum_verts = ltris.size();
    auto const rnum_verts = rtris.size();

    assert(lnum_verts % 3 == 0);
    assert(rnum_verts % 3 == 0);

    for (uint lidx = 0; lidx < lnum_verts; lidx +=3)
    {
        ext<aabb, uint> lbox{ &ltris[lidx], lidx }; 
        if (isect_box.value().intersect(*lbox)) { laabbs.emplace_back(lbox); }
    }

    for (uint ridx = 0; ridx < rnum_verts; ridx += 3)
    {
        ext<aabb, uint> rbox{ &rtris[ridx], ridx };
        if (isect_box.value().intersect(*rbox)) { raabbs.emplace_back(rbox); }
    }

    std::vector<linesegment> result;
    // todo : can optimize this by hashing triangles on the spherical domain
    // may need tiling the sphere with polygons
    for (uint lidx = 0; lidx < laabbs.size(); lidx++)
        for (uint ridx = 0; ridx < raabbs.size(); ridx++)
            if(laabbs[lidx]->intersect(raabbs[ridx]))
                if(auto const& isect = triangle::intersect(&(ltris[laabbs[lidx].ex()]), &rtris[raabbs[ridx].ex()]))
                    result.push_back(isect.value());

    return result;
}

std::vector<vec3> geometry::ffd_object::compute_contacts(ffd_object const& other) const
{
#ifdef PROFILE_TIMING
    static float isect_cost = 0.f;
    auto start = std::chrono::high_resolution_clock::now();
#endif
    auto const lines = intersect(other);

#ifdef PROFILE_TIMING
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    isect_cost += duration.count();

    char buf[512];
    sprintf_s(buf, "Intersection took [%f] ms\n", isect_cost);

    OutputDebugStringA(buf);
#endif

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
    return { gfx::instance_data{matrix::CreateTranslation(center), vec3{1.f,1.f,1.f}} };
}

vec3 transform_point_local(vec3 const& x, vec3 const& y, vec3 const& origin, vec3 const& point)
{
    vec3 z = x.Cross(y);
    z.Normalize();

    matrix tx{ x, z, y };
    tx = tx.Invert();
    return vec3::Transform(point - origin, tx);
}

vec3 transform_point_global(vec3 const& x, vec3 const& y, vec3 const& origin, vec3 const& point)
{
    vec3 z = x.Cross(y);
    z.Normalize();

    matrix tx{ x, z, y };
    return vec3::Transform(point, tx) + origin;
}

vec3 transform_point_global(vec3 const& x, vec3 const& y, vec3 const& origin, vec2 const& point)
{
    return transform_point_global(x, y, origin, vec3(point));
}