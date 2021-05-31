#include "stdafx.h"
#include "Shapes.h"

#include <algorithm>

#ifdef PROFILE_TIMING
#include <chrono>
#endif 

using namespace DirectX;
using namespace Geometry;

std::vector<Vertex> const& sphere::get_triangles() const
{
    return triangulated_sphere;
}

std::vector<linesegment> Geometry::sphere::intersect(sphere const& r) const
{
    return intersect(*this, r);
}

void sphere::addquad(float theta, float phi, float step_theta, float step_phi)
{
    // Create the quad(two triangles) using the vertices
    Vertex lbv, ltv, rbv, rtv;
    ltv = create_spherevertex(phi, theta);
    rtv = create_spherevertex(phi + step_phi, theta);
    lbv = create_spherevertex(phi, theta + step_theta);
    rbv = create_spherevertex(phi + step_phi, theta + step_theta);

    if (!utils::are_equal(lbv.position, ltv.position) && !utils::are_equal(lbv.position, rtv.position) && !utils::are_equal(ltv.position, rtv.position))
    {
        triangulated_sphere.push_back(lbv);
        triangulated_sphere.push_back(ltv);
        triangulated_sphere.push_back(rtv);
    }

    if (!utils::are_equal(lbv.position, rtv.position) && !utils::are_equal(lbv.position, rbv.position) && !utils::are_equal(rtv.position, rbv.position))
    {
        triangulated_sphere.push_back(lbv);
        triangulated_sphere.push_back(rtv);
        triangulated_sphere.push_back(rbv);
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

    float const theta_end = XM_PI - step_radians + utils::tolerance<float>;
    float const phi_end = XM_2PI - step_radians + utils::tolerance<float>;
    float const step_theta = XM_PI - std::floor(XM_PI * reciprocal_stepradians) * step_radians;
    float const step_phi = XM_2PI - std::floor(XM_2PI * reciprocal_stepradians) * step_radians;

    // Divide the sphere into quads for each region defined by four circles(at theta, theta + step, phi and phi + step)
    for (theta = 0.f; theta < theta_end; theta += step_radians)
    {
        for (phi = 0.f; phi < phi_end; phi += step_radians)
        {
            addquad(theta, phi, step_radians, step_radians);
        }

        if (step_phi > utils::tolerance<float>)
        {
            // what are you doing step phi?
            // step phi is not a factor of 2PI, so make the last step smaller
            addquad(theta, phi, step_radians, step_phi);
        }
    }

    if (step_theta > utils::tolerance<float>)
    {
        for (phi = 0.f; phi < phi_end; phi += step_radians)
        {
            addquad(theta, phi, step_theta, step_radians);
        }

        if (step_phi > utils::tolerance<float>)
        {
            addquad(theta, phi, step_theta, step_phi);
        }
    }
}

Vertex sphere::create_spherevertex(float const phi, float const theta)
{
    Vertex res;
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

void ffd_object::update(float dt)
{
    box = std::move(aabb{ control_points });
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

std::vector<Vector3> ffd_object::get_control_point_visualization() const
{
    auto const marker_vec3 = utils::create_marker(Vector3::Zero, 0.1f);

    std::vector<Vector3> marker;
    marker.reserve(marker_vec3.size());
    for (auto const& point : marker_vec3)
    {
        marker.push_back(point);
    }

    return marker;
}

Vector3 Geometry::ffd_object::compute_wholebody_forces() const
{
    auto const drag = -velocity * 0.5f;
    return drag;
}

void Geometry::ffd_object::set_velocity(Vector3 const vel)
{
    velocity = vel;
}

void Geometry::ffd_object::resolve_collision(ffd_object & r, float dt)
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

    // todo : try distributong the impulse to neighbouring control points as well, slightly reduced
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

uint Geometry::ffd_object::closest_controlpoint(Vector3 const& point) const
{
    uint ctrlpt_idx = utils::invalid<uint>();
    float distsqr_min = std::numeric_limits<float>::max();
    for (uint i = 0; i < num_control_points; ++i)
    {
        if (auto distsqr = Vector3::DistanceSquared(point, control_points[i]); distsqr < distsqr_min)
        {
            distsqr_min = distsqr;
            ctrlpt_idx = i;
        }
    }

    return ctrlpt_idx;
}

std::vector<linesegment> Geometry::ffd_object::intersect(ffd_object const& r) const
{
    auto const& ltris = get_physx_traingles();
    auto const& rtris = r.get_physx_traingles();

    auto const lnum_verts = ltris.size();
    auto const rnum_verts = rtris.size();

    assert(lnum_verts % 3 == 0);
    assert(rnum_verts % 3 == 0);

    std::vector<linesegment> result;
    for(int lidx = 0; lidx < lnum_verts; lidx+=3)
        for (int ridx = 0; ridx < lnum_verts; ridx += 3)
        {
            if (auto const& isect = triangle::intersect(&ltris[lidx], &rtris[ridx]))
            {
                result.push_back(isect.value());
            }
        }

    return result;
}

std::vector<Geometry::Vector3> Geometry::ffd_object::compute_contacts(ffd_object const& other) const
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

    std::vector<Vector3> mids;
    mids.reserve(lines.size() / 2);
    for (uint i = 0; i < lines.size(); i++)
    {
        mids.push_back((lines[i].v0 + lines[i].v1) / 2.f);
    }

    auto constexpr contact_radius = 0.3f;
    auto constexpr contact_radius_squared2 = 2 * contact_radius * contact_radius;
    std::vector<Vector3> contacts;

    // naively cluster points around first point that is not in existing clusters
    for (uint i = 0; i < mids.size(); ++i)
    {
        contacts.push_back(mids[i]);
        for (uint j = i + 1; j < mids.size(); ++j)
        {
            if (Vector3::DistanceSquared(mids[i], mids[j]) < contact_radius_squared2)
            {
                std::swap(mids[j], mids[mids.size() - 1]);
                mids.pop_back();
                j--;
            }
        }
    }

    return contacts;
}

std::vector<Vertex> circle::get_triangles() const
{
    std::vector<Vector3> vertices;

    float x0 = radius * cosf(0);
    float y0 = radius * sinf(0);

    for (float t = step; t <= (XM_2PI + step); t+=step)
    {
        float x = radius * cosf(t);
        float y = radius * sinf(t);

        vertices.push_back(Vector3::Zero);
        vertices.push_back({x, y , 0.f});
        vertices.push_back({x0, y0 , 0.f});

        x0 = x;
        y0 = y;
    }

    Matrix plane_tx = utils::get_planematrix(center, normal);

    std::vector<Vertex> result;
    for (auto& vert : vertices)
    {
        result.push_back({ Vector3::Transform(vert, plane_tx), normal });
    }

    return result;
}

std::vector<gfx::instance_data> circle::get_instance_data() const
{
    return { {center, {1.f,1.f,1.f } } };
}

Vector4 utils::create_Vector4(Vector3 const& v, float w)
{
    return { v.x, v.y, v.z, w };
}

Matrix utils::get_planematrix(Vector3 translation, Vector3 normal)
{
    Vector3 axis1 = normal.Cross(Vector3::UnitX);
    if (axis1.LengthSquared() <= 0.001)
    {
        axis1 = normal.Cross(Vector3::UnitY);
    }

    axis1.Normalize();

    Vector3 axis2 = axis1.Cross(normal);
    axis2.Normalize();

    // treat normal as y axis
    Matrix result = Matrix(axis1, axis2, -normal);
    result.Translation(translation);

    return result;
}

std::vector<Vector3> utils::create_marker(Vector3 point, float scale)
{
    auto apply_transformation = [](std::vector<Vector3> const& points, const Vector3(&tx)[2])
    {
        std::vector<Vector3> transformed_points;
        transformed_points.reserve(points.size());

        auto const angle = tx[0].Length();
        auto axis = tx[0];
        axis.Normalize();

        for (auto const& point : points)
        {
            Vector3 result;
            Vector3::Transform(point, DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(axis, XMConvertToRadians(angle)), result);
            result += tx[1];

            transformed_points.push_back(std::move(result));
        }

        return transformed_points;
    };

    auto create_unit_marker = [&apply_transformation]()
    {
        static constexpr Vector3 transformations[6][2] =
        {
            {{0.f, 0.f, 360.f}, {0.f, 0.0f, 0.5f}},
            {{0.f, 0.f, 360.f}, {0.f, 0.0f, -0.5f}},
            {{90.f, 0.f, 0.f}, {0.f, 0.5f, 0.f}},
            {{90.f, 0.f, 0.f}, {0.f, -0.5f, 0.f}},
            {{0.f, 90.f, 0.f}, {0.5f, 0.f, 0.f}},
            {{0.f, 90.f, 0.f}, {-0.5f, 0.0f, 0.f}}
        };

        static std::vector<Vector3> unit_frontface_quad =
        {
            { -0.5f, 0.5f, 0.f },
            { 0.5f, 0.5f, 0.f },
            { 0.5f, -0.5f, 0.f },
            { -0.5f, -0.5f, 0.f }
        };

        std::vector<Vector3> unit_marker_lines;
        for (auto const& transformation : transformations)
        {
            auto const face = apply_transformation(unit_frontface_quad, transformation);
            auto const num_verts = face.size();
            for (size_t vert_idx = 0; vert_idx < num_verts; ++vert_idx)
            {
                unit_marker_lines.push_back(face[vert_idx]);
                unit_marker_lines.push_back(face[(vert_idx + 1) % num_verts]);
            }
        }

        return unit_marker_lines;
    };

    static std::vector<Vector3> unit_marker = create_unit_marker();

    std::vector<Vector3> result;
    result.reserve(unit_marker.size());
    for (auto const vertex : unit_marker)
    {
        result.push_back((vertex * scale) + point);
    }

    return result;
}

std::vector<Vector3> utils::flatten(std::vector<linesegment> const& lines)
{
    std::vector<Vector3> result;
    result.reserve(lines.size() * 2);
    for (auto const& line : lines)
    {
        result.push_back(line.v0);
        result.push_back(line.v1);
    }

    return result;
}

bool utils::are_equal(float const& l, float const& r)
{
    return std::fabsf(l - r) < tolerance<float>;
}

bool utils::are_equal(Vector3 const& l, Vector3 const& r)
{
    return std::fabsf(l.x - r.x) < tolerance<float> && std::fabsf(l.y - r.y) < tolerance<float> && std::fabsf(l.z - r.z) < tolerance<float>;
}

Vector3 transform_point_local(Vector3 const& x, Vector3 const& y, Vector3 const& origin, Vector3 const& point)
{
    Vector3 z = x.Cross(y);
    z.Normalize();

    Matrix tx{ x, z, y };
    tx = tx.Invert();
    return Vector3::Transform(point - origin, tx);
}

Vector3 transform_point_global(Vector3 const& x, Vector3 const& y, Vector3 const& origin, Vector3 const& point)
{
    Vector3 z = x.Cross(y);
    z.Normalize();

    Matrix tx{ x, z, y };
    return Vector3::Transform(point, tx) + origin;
}

Vector3 transform_point_global(Vector3 const& x, Vector3 const& y, Vector3 const& origin, Vector2 const& point)
{
    return transform_point_global(x, y, origin, Vector3(point));
}

bool triangle2D::isin(Vector2 const& point) const
{
    Vector2 vec0 = v1 - v0, vec1 = v2 - v0, vec2 = point - v0;
    float den = vec0.x * vec1.y - vec1.x * vec0.y;
    float v = (vec2.x * vec1.y - vec1.x * vec2.y) / den;
    float w = (vec0.x * vec2.y - vec2.x * vec0.y) / den;
    float u = 1.0f - v - w;

    return (u >= 0.f && u <= 1.f) && (v >= 0.f && v <= 1.f) && (w >= 0.f && w <= 1.f);
}

bool Geometry::triangle::isin(Vector3 const* tri, Vector3 const& point)
{
    Vector3 normal_unnormalized = (tri[1] - tri[0]).Cross(tri[2] - tri[0]);
    Vector3 normal = normal_unnormalized;
    normal.Normalize();

    float const triarea = normal.Dot(normal_unnormalized);
    if (triarea == 0)
        return false;

    float const alpha = normal.Dot((tri[1] - point).Cross(tri[2] - point)) / triarea;
    float const beta = normal.Dot((tri[2] - point).Cross(tri[0] - point)) / triarea;

    return alpha > -utils::tolerance<float> && beta > -utils::tolerance<float> && (alpha + beta) < (1.f + utils::tolerance<float>);
}

std::optional<linesegment> triangle::intersect(triangle const& t0, triangle const& t1)
{
    return intersect(t0.verts, t1.verts);
}

std::optional<linesegment> Geometry::triangle::intersect(Vector3 const* t0, Vector3 const* t1)
{
    Vector3 t0_normal = (t0[1] - t0[0]).Cross(t0[2] - t0[0]);
    Vector3 t1_normal = (t1[1] - t1[0]).Cross(t1[2] - t1[0]);
    t0_normal.Normalize();
    t1_normal.Normalize();

    // construct third plane as cross product of the two plane normals and passing through origin
    auto linedir = t0_normal.Cross(t1_normal);
    linedir.Normalize();

    // use 3 plane intersection to find a point on the inerseciton line
    auto const det = Matrix(t0_normal, t1_normal, linedir).Determinant();

    if (fabs(det) < FLT_EPSILON)
        return {};

    Vector3 const& linepoint0 = (t0[0].Dot(t0_normal) * (t1_normal.Cross(linedir)) + t1[0].Dot(t1_normal) * linedir.Cross(t0_normal)) / det;

    line const isect_line{ linepoint0, linedir };

    std::vector<line> edges;
    edges.reserve(6);

    auto dir = (t0[1] - t0[0]);
    dir.Normalize();
    edges.emplace_back(line{ t0[0], dir });
    dir = (t0[2] - t0[1]);
    dir.Normalize();
    edges.emplace_back(line{ t0[1], dir });
    dir = (t0[0] - t0[2]);
    dir.Normalize();
    edges.emplace_back(line{ t0[2], dir });

    dir = (t1[1] - t1[0]);
    dir.Normalize();
    edges.emplace_back(line{ t1[0], dir });
    dir = (t1[2] - t1[1]);
    dir.Normalize();
    edges.emplace_back(line{ t1[1], dir });
    dir = (t1[0] - t1[2]);
    dir.Normalize();
    edges.emplace_back(line{ t1[2], dir });

    std::vector<Vector3> isect_linesegment;
    isect_linesegment.reserve(2);

    for (auto const& edge : edges)
    {
        // ignore intersections with parallel edge
        if (utils::are_equal(std::fabsf(edge.dir.Dot(isect_line.dir)), 1.f))
            continue;

        // the points on intersection of two triangles will be on both triangles
        if (auto isect = line::intersect_lines(edge, isect_line); isect.has_value() && triangle::isin(t0, isect.value()) && triangle::isin(t1, isect.value()))
            isect_linesegment.push_back(isect.value());
    }

    // remove duplicates, naively, maybe improve this
    for (uint i = 0; i < std::max(uint(1), isect_linesegment.size()) - 1; ++i)
        for (uint j = i + 1; j < isect_linesegment.size(); ++j)
            if (utils::are_equal(isect_linesegment[i], isect_linesegment[j]))
            {
                std::swap(isect_linesegment[j], isect_linesegment[isect_linesegment.size() - 1]);
                isect_linesegment.pop_back();
                j--;
            }

    if (isect_linesegment.size() < 2)
        return {};

    //assert(isect_linesegment.size() != 2);

    // there is an issue here
    /*if (isect_linesegment.size() != 2)
    {
        DebugBreak();
    }*/

    return { { isect_linesegment[0], isect_linesegment[1] } };
}

std::optional<Vector2> linesegment::intersect_line2D(linesegment const& linesegment, line const& line)
{
    auto cross = [](Vector2 const& l, Vector2 const& r) { return l.x * r.y - l.y * r.x; };
    // point = p + td1
    // point = q + ud2

    Vector2 p(linesegment.v0), q(line.point);
    Vector2 d1(linesegment.v1 - linesegment.v0), d2(line.dir);
    d1.Normalize();

    if (float d1crossd2 = cross(d1, d2); d1crossd2 != 0.f)
    {
        float t = cross(q - p, d2) / d1crossd2;

        return p + t * d1;
    }

    return {};
}

line2D line::to2D() const
{
    return { point.To2D(), dir.To2D() };
}

float line2D::getparameter(line2D const& line, Vector2 const& point)
{
    // assuming point is already on the line
    auto origin = line.point;
    auto topoint = point - origin;
    topoint.Normalize();

    return Vector2::Distance(origin, point) * (topoint.Dot(line.dir) > 0.f ? 1.f : -1.f);
}

float line::getparameter(line const& line, Vector3 const& point)
{
    // assuming point is already on the line
    auto origin = line.point;
    auto topoint = point - origin;
    topoint.Normalize();

    return Vector3::Distance(origin, point) * (topoint.Dot(line.dir) > 0.f ? 1.f : -1.f);
}

std::optional<Vector3> Geometry::line::intersect_lines(line const& l, line const& r)
{
    // if the lines intersect they need to be on the same plane.
    // calculate plane normal
    auto n = l.dir.Cross(r.dir);
    n.Normalize();

    // if lines intersect dot product to normal and any point on both lines is constant
    auto const c1 = n.Dot(l.point);
    if(std::fabs(c1 - n.Dot(r.point)) > utils::tolerance<float>)
        return {};

    // calculate cross of n and line dir
    auto ldir_cross_n = l.dir.Cross(n);
    ldir_cross_n.Normalize();

    // this constant should be same for all points on l, i.e 
    auto const c = ldir_cross_n.Dot(l.point);

    // c = ldir_cross_n.Dot(isect) and isect = r.point + r.dir * t2
    auto const t2 = (c - ldir_cross_n.Dot(r.point)) / ldir_cross_n.Dot(r.dir);

    return { r.point + t2 * r.dir };
}
