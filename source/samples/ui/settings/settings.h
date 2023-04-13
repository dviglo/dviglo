// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../sample.h"

namespace dviglo
{

class Window;
class DropDownList;
class CheckBox;

}

/// Demo application for dynamic window settings change.
class WindowSettingsDemo : public Sample
{
    DV_OBJECT(WindowSettingsDemo);

public:
    /// Construct.
    explicit WindowSettingsDemo();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void create_scene();
    /// Create window with settings.
    void InitSettings();
    /// Synchronize settings with current state of the engine.
    void SynchronizeSettings();

    /// The Window.
    Window* window_{};
    /// The UI's root UiElement.
    UiElement* uiRoot_{};

    /// Monitor control.
    DropDownList* monitorControl_{};
    /// Resolution control.
    DropDownList* resolutionControl_{};
    /// Fullscreen control.
    CheckBox* fullscreenControl_{};
    /// Borderless flag control.
    CheckBox* borderlessControl_{};
    /// Resizable flag control.
    CheckBox* resizableControl_{};
    /// V-sync flag control.
    CheckBox* vsyncControl_{};
    /// MSAA control.
    DropDownList* multiSampleControl_{};
};


