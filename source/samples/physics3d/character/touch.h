// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/core/object.h>

using namespace dviglo;

namespace dviglo
{
    class Controls;
}

const float CAMERA_MIN_DIST = 1.0f;
const float CAMERA_INITIAL_DIST = 5.0f;
const float CAMERA_MAX_DIST = 20.0f;

/// Mobile framework for Android/iOS
/// Gamepad from NinjaSnowWar
/// Touches patterns:
///     - 1 finger touch  = pick object through raycast
///     - 1 or 2 fingers drag  = rotate camera
///     - 2 fingers sliding in opposite direction (up/down) = zoom in/out
///
/// Setup:
/// - Call the update function 'UpdateTouches()' from HandleUpdate or equivalent update handler function
class Touch : public Object
{
    DV_OBJECT(Touch, Object);

public:
    /// Construct.
    Touch(Context* context, float touchSensitivity);
    /// Destruct.
    ~Touch() override;

    /// Update touch controls for the current frame.
    void UpdateTouches(Controls& controls);

    /// Touch sensitivity.
    float touchSensitivity_;
    /// Current camera zoom distance.
    float cameraDistance_;
    /// Zoom flag.
    bool zoom_;
    /// Gyroscope on/off flag.
    bool useGyroscope_;
};

