#include "cursor.h"
#include "engine/graphics/gfxutils.h"
#include "engine/win32application.h"

using viewport = DirectX::SimpleMath::Viewport;

void cursor::tick(float dt)
{
    _lastpos = _pos;
    _pos = pos();
    _vel = (_pos - _lastpos) / dt;
    _vel.y = -_vel.y;     // pos is relative to topleft so invert y
}

vec2 cursor::pos() const
{
    POINT pos;
    GetCursorPos(&pos);
    ScreenToClient(Win32Application::GetHwnd(), &pos);
    return { static_cast<float>(pos.x), static_cast<float>(pos.y) };
}

vec2 cursor::vel() const { return _vel; }

vec3 cursor::ray(float nearp, float farp) const
{
    auto const raystart = to3d({ _pos.x, _pos.y, nearp }, nearp, farp);
    auto const rayend = to3d({ _pos.x, _pos.y, farp }, nearp, farp);

    return (rayend - raystart).Normalized();
}

vec3 cursor::to3d(vec3 pos, float nearp, float farp) const
{
    RECT clientr;
    GetClientRect(Win32Application::GetHwnd(), &clientr);

    float const width = static_cast<float>(clientr.right - clientr.left);
    float const height = static_cast<float>(clientr.bottom - clientr.top);

    auto const& view = gfx::getview();

    // frustum z range = 1000.f
    // convert to ndc [-1, 1]
    vec4 const ndc = vec4{ pos.x / width, (height - pos.y - 1.f) / height, (pos.z - nearp) / (farp - nearp) , 1.f } * 2.f - vec4{ 1.f };

    // homogenous space
    auto posh = vec4::Transform(ndc, view.proj.Invert());
    posh /= posh.w;

    // world space
    posh = vec4::Transform(posh, view.view.Invert());

    return { posh.x, posh.y, posh.z };
}