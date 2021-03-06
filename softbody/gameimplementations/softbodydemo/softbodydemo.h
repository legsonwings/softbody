#pragma once

#include "gamebase.h"
#include "engine/geometry/ffd.h"
#include "engine/graphics/gfxcore.h"

import shapes;
class game_engine;

namespace gfx
{
	struct instance_data;

	template<sbody_c body_t, gfx::topology primitive_t = gfx::topology::triangle>
	class body_static;

	template<dbody_c body_t, gfx::topology primitive_t = gfx::topology::triangle>
	class body_dynamic;
}

class soft_body final : public game_base
{
public:
	soft_body(gamedata const& data);

	void update(float dt) override;
	void render(float dt) override;
	
	gfx::resourcelist load_assets_and_geometry() override;

	void on_key_down(unsigned key) override;

private:
	bool wireframe_toggle = false;
	bool debugviz_toggle = false;

	std::vector<gfx::body_dynamic<geometry::ffd_object>> balls;
	std::vector<gfx::body_static<geometry::cube>> boxes;
	std::vector<gfx::body_dynamic<geometry::ffd_object const&, gfx::topology::line>> reflines;
	std::vector<gfx::body_static<geometry::ffd_object const&, gfx::topology::line>> refstaticlines;
};