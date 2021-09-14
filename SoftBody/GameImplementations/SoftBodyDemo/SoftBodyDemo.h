#pragma once

#include "gamebase.h"
#include "engine/SimpleMath.h"
#include "engine/SimpleCamera.h"
#include "engine/graphics/gfxmemory.h"
#include "Engine/geometry/ffd.h"
#include "Engine/geometry/beziershapes.h"

import shapes;
class game_engine;

namespace gfx
{
	struct instance_data;

	template<typename geometry_t, gfx::topology primitive_t = gfx::topology::triangle>
	class body_static;

	template<typename geometry_t, gfx::topology primitive_t = gfx::topology::triangle>
	class body_dynamic;
}

class soft_body final : public game_base
{
public:
	soft_body(game_engine const* engine);

	void update(float dt) override;
	void render(float dt) override;
	
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> load_assets_and_geometry() override;

	void on_key_down(unsigned key) override;
	void on_key_up(unsigned key) override;

private:
	bool wireframe_toggle = false;
	bool debugviz_toggle = false;

	SimpleCamera m_camera;    
	constant_buffer cbuffer;

	std::vector<gfx::body_dynamic<geometry::ffd_object>> balls;
	std::vector<gfx::body_static<geometry::cube>> boxes;
	std::vector<gfx::body_dynamic<geometry::ffd_object const&, gfx::topology::line>> reflines;
	std::vector<gfx::body_static<geometry::ffd_object const&, gfx::topology::line>> refstaticlines;
};