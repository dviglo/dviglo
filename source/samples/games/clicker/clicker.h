// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include <dviglo/math/big_int.h>

#include "../../Sample.h"

class Clicker : public Sample
{
    DV_OBJECT(Clicker, Sample);

public:
    /// Construct.
    explicit Clicker();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Current score.
    BigInt score_;

    /// Number of points received per click.
    BigInt power_{1};

    /// Delay after click.
    float clickDelay_ = 0.f;

    /// Construct UI elements.
    void CreateUI();
    
    /// Subscribe to application-wide logic update events.
    void subscribe_to_events();
    
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);

    /// Handle the mouse click event.
    void HandleMouseButtonDown(StringHash eventType, VariantMap& eventData);
};
