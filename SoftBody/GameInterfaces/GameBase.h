#pragma once

#include <string>
#include <array>
#include <vector>
#include <memory>

class game_engine;

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

	virtual void on_key_down(unsigned key) {};
	virtual void on_key_up(unsigned key) {};

protected:
	game_engine const * engine;
};