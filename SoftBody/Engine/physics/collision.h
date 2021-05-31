#pragma once

#include "Engine/SimpleMath.h"
#include "DirectXMath.h"
#include "Engine/EngineUtils.h"
#include "Engine/Shapes.h"

#include <vector>
//#include <array>
//#include <limits>
//#include <cmath>
//#include <optional>
//#include <algorithm>
//#include <functional>

namespace collision
{
    using Vector2 = DirectX::SimpleMath::Vector2;
    using Vector3 = DirectX::SimpleMath::Vector3;
    using Vector4 = DirectX::SimpleMath::Vector4;
    using Matrix = DirectX::SimpleMath::Matrix;

    class spatial_partition
    {
    public:
        spatial_partition() = default;
        spatial_partition(float _gridsize, Geometry::aabb const& _space_bounds) : gridsize(_gridsize), space_bounds(_space_bounds) {}
        void update(std::vector<Vector3> const &tris, float dt);

    private:
        float gridsize = 1.f;
        Geometry::aabb space_bounds;

        static constexpr std::size_t storage_size = 256;
    };
}