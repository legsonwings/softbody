#pragma once

#include "geocore.h"
#include "Engine/EngineUtils.h"
#include "line.h"

#include <utility>

namespace geometry
{
    struct sphere
    {
        using polar_coords = std::pair<float, float>;

        sphere() { generate_triangles(); }
        sphere(vec3 const& _position, float _radius) : position(_position), radius(_radius) { generate_triangles(); }

        void generate_triangles();
        std::vector<vertex> const& get_triangles() const;
        std::vector<polar_coords> const& get_triangles_polar() const;
        std::vector<linesegment> intersect(sphere const& r) const;
        static std::vector<linesegment> intersect(sphere const& l, sphere const& r);

    private:
        void sort_tris();
        void addquad(float theta, float phi, float step_theta, float step_phi);
        vertex create_spherevertex(float const phi, float const theta);

        std::vector<vertex> triangulated_sphere;
        std::vector<polar_coords> triangulated_sphere_polar;
    public:

        // todo : use num segments instead
        float step_degrees = 30.f;
        float radius = 1.5f;
        vec3 position = {};
    };
}