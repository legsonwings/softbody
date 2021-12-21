#pragma once

#include "engine/ui/uicore.h"
#include "engine/graphics/gfxcore.h"
#include "engine/geometry/geocore.h"
#include "engine/graphics/globalresources.h"
#include "engine/core.h"

import shapes;

// todo : add concept
template<typename t>
struct shapebody
{
    shapebody() = default;
	shapebody(t body, vector3 center, gfx::material const& mat) : _center(center), _mat(mat)
	{
		_tris = std::move(body.triangles());
	}

	// todo : make getter for this
	geometry::aabb const& gaabb() const { return _box; }
	std::vector<geometry::vertex> triangles() const { return _tris; }
	std::vector<gfx::instance_data> instancedata() const { return { gfx::instance_data(matrix::CreateTranslation(_center), gfx::globalresources::get().view(), _mat) }; }

	geometry::aabb _box;
	vector3 _center;
	gfx::material _mat;
	std::vector<geometry::vertex> _tris;
};

namespace ui
{
class canvas
{
    shapebody<geometry::rectangle> backgroundbox{{2, 2}, vector3::Zero, gfx::material().diffuse(gfx::color::black)};

    std::vector<canvasslot> _children;
};
}
