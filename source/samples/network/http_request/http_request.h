// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../Sample.h"

namespace dviglo
{
    class HttpRequest;
    class Text;
}

/// Http request example.
/// This example demonstrates:
///     - How to use Http request API
class HttpRequestDemo : public Sample
{
    DV_OBJECT(HttpRequestDemo, Sample);

public:
    /// Construct.
    explicit HttpRequestDemo();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Create the user interface.
    void CreateUI();
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);

    String message_;
    SharedPtr<Text> text_;
    SharedPtr<HttpRequest> httpRequest_;
};
