module;

#include "engine/core.h"
#include <cmath> 

export module spring;

export namespace physx
{
// simulates shm
struct spring
{
    std::pair<vector3, vector3> dummy(vector3 const& displacement, vector3 const& vel, float dt) const { return { displacement, vel, }; }

    std::pair<vector3, vector3> damped(vector3 const& displacement, vector3 const& vel, float dt) const
    {
        std::pair<vector3, vector3> res;

        res.first = std::powf(2.71828f, -omega * eta * dt) * (displacement * std::cos(alpha * dt) + (vel + omega * eta * displacement) * std::sin(alpha * dt) / alpha);
        res.second = -std::pow(2.71828f, -omega * eta * dt) * ((displacement * omega * eta - vel - omega * eta * displacement) * cos(alpha * dt) +
                    (displacement * alpha + (vel+ omega * eta * displacement) * omega * eta / alpha) * sin(alpha * dt));
        return res;
    }

    std::pair<vector3, vector3> critical(vector3 const& displacement, vector3 const& vel, float dt) const
    {
        std::pair<vector3, vector3> res;

        res.first = ((vel + displacement * omega) * dt + displacement) * std::powf(2.71828f, -omega * dt);
        res.second = (vel - (vel + displacement * omega) * omega * dt) * std::powf(2.71828f, -omega * dt);
        return res;
    }

    // eta = damping factor
    // omega = std::sqrt(spring_const / mass);
    static constexpr float eta = 0.5f;
    static constexpr float springconst = 7.29f;
    float const omega = std::sqrt(springconst / 1.f);
    float const alpha = omega * std::sqrt(1 - eta * eta);
};
}