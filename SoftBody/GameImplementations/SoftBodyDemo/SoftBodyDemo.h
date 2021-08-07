#pragma once

#include "gamebase.h"
#include "engine/SimpleMath.h"
#include "engine/SimpleCamera.h"
#include "engine/graphics/gfxmemory.h"
#include "Engine/geometry/shapes.h"
#include "engine/geometry/cube.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

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
	
	std::vector<ComPtr<ID3D12Resource>> load_assets_and_geometry() override;

	void switch_cameraview();
	void on_key_down(unsigned key) override;
	void on_key_up(unsigned key) override;

private:
	bool m_wireframe_toggle = false;
	bool m_debugviz_toggle = true;

	uint8_t camera_view = 0;

	SimpleCamera m_camera;    
	constant_buffer cbuffer;

	std::vector<gfx::body_dynamic<geometry::ffd_object>> balls;
	std::vector<gfx::body_dynamic<geometry::ffd_object>> bubbles;
	std::vector<gfx::body_static<geometry::cube>> staticbodies_boxes;
	std::vector<gfx::body_dynamic<geometry::ffd_object const&, gfx::topology::line>> dynamicbodies_line;
	std::vector<gfx::body_static<geometry::ffd_object const&, gfx::topology::line>> staticbodies_lines;
};