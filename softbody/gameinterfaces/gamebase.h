#pragma once

#include <string>
#include <array>
#include <vector>
#include <memory>

#define NOMINMAX
#include <wrl.h>

#include "engine/graphics/gfxmemory.h"
#include "engine/SimpleCamera.h"

class game_engine;
struct ID3D12Resource;

// abstract base game class
class game_base
{
public:
	using resourcelist = std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>;

	game_base(game_engine const * _engine);
	
	virtual ~game_base() {}

	virtual resourcelist load_assets_and_geometry() = 0;

	virtual void update(float dt) = 0;
	virtual void render(float dt) = 0;

	virtual void on_key_down(unsigned key) { m_camera.OnKeyDown(key); };
	virtual void on_key_up(unsigned key) { m_camera.OnKeyUp(key); };

protected:
	game_engine const * engine;
	SimpleCamera m_camera;
	constant_buffer cbuffer;
};