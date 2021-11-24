//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#define NOMINMAX
#include <wtypes.h>
#include <DirectXMath.h>

class SimpleCamera
{
public:
    SimpleCamera();

    void Init(DirectX::XMFLOAT3 position);
    void Update(float elapsedSeconds);
    DirectX::XMFLOAT3 GetCurrentPosition() const;
    DirectX::XMMATRIX GetViewMatrix();
    DirectX::XMMATRIX GetProjectionMatrix(float fov);
    DirectX::XMMATRIX GetOrthoProjectionMatrix();

    void lock(bool lock);

    float nearplane() const;
    float farplane() const;
    void nearplane(float nearp);
    void farplane(float farp);
    void width(unsigned width);
    void height(unsigned height);

    void SetMoveSpeed(float unitsPerSecond);
    void SetTurnSpeed(float radiansPerSecond);

    void TopView();
    void BotView();
    void OnKeyDown(WPARAM key);
    void OnKeyUp(WPARAM key);
private:
    void Reset();

    struct KeysPressed
    {
        bool w;
        bool a;
        bool s;
        bool d;

        bool left;
        bool right;
        bool up;
        bool down;
    };

    DirectX::XMFLOAT3 m_initialPosition;
    DirectX::XMFLOAT3 m_position;
    float m_yaw;                // Relative to the +z axis.
    float m_pitch;                // Relative to the xz plane.
    DirectX::XMFLOAT3 m_lookDirection;
    DirectX::XMFLOAT3 m_upDirection;
    float m_moveSpeed;            // Speed at which the camera moves, in units per second.
    float m_turnSpeed;            // Speed at which the camera turns, in radians per second.

    bool _locked = false;
    float _nearp = 0.01f;
    float _farp = 1000.f;
    unsigned _width = 0;
    unsigned _height = 0;

    KeysPressed m_keysPressed;
};
