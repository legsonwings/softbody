#include "cursor.h"
#include "engine/graphics/gfxutils.h"
#include "engine/win32application.h"

using viewport = DirectX::SimpleMath::Viewport;

// todo : store near and far planes as vars
static constexpr float neard = 0.01f;
static constexpr float fard = 1000.f;

void cursor::tick(float dt)
{
    _lastpos = _pos;
    _pos = pos();
    _vel = (_pos - _lastpos) / dt;
}

vec2 cursor::pos() const
{
    POINT pos;
    GetCursorPos(&pos);
    ScreenToClient(Win32Application::GetHwnd(), &pos);

    return { static_cast<float>(pos.x), static_cast<float>(pos.y) };
}

vec2 cursor::vel() const { return _vel; }

vec3 cursor::ray() const
{
    auto const raystart = to3d({ _pos.x, _pos.y, neard });
    auto const rayend = to3d({ _pos.x, _pos.y, fard });

    return (rayend - raystart).Normalized();
}

vec3 cursor::to3d(vec3 screenpos) const
{
    auto const& view = gfx::getview();

    // frustum z range = 1000.f
    // convert to ndc [-1, 1]

    // todo : get screen width and height
    vec4 const ndc = vec4{ screenpos.x / 1280.f, (720.f - screenpos.y - 1.f) / 720.f, (screenpos.z - neard) / (fard - neard) , 1.f } * 2.f - vec4{ 1.f };

    // homogenous space
    auto pos = vec4::Transform(ndc, view.proj.Invert());
    pos /= pos.w;

    // world space
    pos = vec4::Transform(pos, view.view.Invert());

    return { pos.x, pos.y, pos.z };
}
