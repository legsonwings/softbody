#pragma once

#include "Engine/SimpleMath.h"
#include "DirectXMath.h"
#include "Engine/engineutils.h"
#include "Engine/geometry/Shapes.h"

#include <vector>

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
        spatial_partition(float _gridsize, geometry::aabb const& _space_bounds) : gridsize(_gridsize), space_bounds(_space_bounds) {}
        void update(std::vector<Vector3> const &tris, float dt);

    private:
        float gridsize = 1.f;
        geometry::aabb space_bounds;

        static constexpr std::size_t storage_size = 256;
    };
}