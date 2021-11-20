#pragma once

#include "Engine/SimpleMath.h"
#include "DirectXMath.h"
#include "Engine/engineutils.h"
#include "engine/core.h"
#include "engine/geometry/geocore.h"

#include <vector>

namespace collision
{
    class spatial_partition
    {
    public:
        spatial_partition() = default;
        spatial_partition(float _gridsize, geometry::aabb const& _space_bounds) : gridsize(_gridsize), space_bounds(_space_bounds) {}
        void update(std::vector<vector3> const &tris, float dt);

    private:
        float gridsize = 1.f;
        geometry::aabb space_bounds;

        static constexpr std::size_t storage_size = 256;
    };
}