#include "stdafx.h"
#include "cube.h"
#include "geoutils.h"
#include "engine/graphics/gfxutils.h"

using namespace geometry;

std::vector<vertex> cube::get_vertices() const
{
    return geoutils::create_cube(center, extents);
}

std::vector<vertex> cube::get_vertices_invertednormals() const
{
    auto verts = geoutils::create_cube(center, extents);
    auto const num_verts = verts.size();
    assert(num_verts % 3 == 0);

    // todo : pso cannot be changed per render object right now, so just reorder the triangles
    // alternative1 : set a pso with reversed cull mode
    // alternative2 : supply a negative scale to shader
    for (uint i = 0; i < num_verts; i+=3) 
    { 
        verts[i].normal *= -1.f;
        verts[i + 1].normal *= -1.f;
        verts[i + 2].normal *= -1.f;
        std::swap(verts[i], verts[i + 2]); 
    }
    
    return verts;
}

std::vector<gfx::instance_data> geometry::cube::getinstancedata() const
{
    return { gfx::instance_data(matrix::CreateTranslation(center), gfx::getmat("room"))};
}
