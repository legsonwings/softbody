#pragma once

#include <string>
#include <array>
#include <vector>
#include <memory>

#define NOMINMAX
#include <wrl.h>

#include "engine/engineutils.h"
#include "engine/graphics/gfxmemory.h"
#include "engine/SimpleCamera.h"

struct ID3D12Resource;

// abstract base game class
class game_base
{
public:
	using resourcelist = std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>;

	game_base(gamedata const &data);
	
	virtual ~game_base() {}

	virtual resourcelist load_assets_and_geometry() = 0;

	virtual void update(float dt) { updateview(dt); };
	virtual void render(float dt) = 0;

	virtual void on_key_down(unsigned key) { camera.OnKeyDown(key); };
	virtual void on_key_up(unsigned key) { camera.OnKeyUp(key); };

protected:
	SimpleCamera camera;
	constant_buffer cbuffer;

	void updateview(float dt);
};