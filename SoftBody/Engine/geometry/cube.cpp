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
    return geoutils::create_cube(center, -extents);
}

std::vector<gfx::instance_data> geometry::cube::getinstancedata() const
{
    return { gfx::instance_data(matrix::CreateTranslation(center), gfx::getview(), gfx::getmat("room"))};
}
