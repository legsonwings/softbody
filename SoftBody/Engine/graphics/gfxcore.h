#pragma once

#include "Engine/SimpleMath.h"

using namespace DirectX;

namespace gfx
{
    enum class topology
    {
        triangle,
        line
    };

    struct instance_data
    {
        DirectX::SimpleMath::Vector3 position = {};
        DirectX::SimpleMath::Vector3 color = { 1.f, 0.f, 0.f };
    };
}