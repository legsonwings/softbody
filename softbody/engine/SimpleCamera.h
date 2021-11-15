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
    DirectX::XMMATRIX GetProjectionMatrix(float fov, float aspectRatio, float nearPlane = 0.1f, float farPlane = 1000.0f);
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

    KeysPressed m_keysPressed;
};
