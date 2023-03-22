// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../Sample.h"

namespace dviglo
{

class Window;

}

/// A 3D UI demonstration based on the HelloGUI sample. Renders UI alternatively
/// either to a 3D scene object using UIComponent, or directly to the backbuffer.
class Hello3DUI : public Sample
{
    DV_OBJECT(Hello3DUI, Sample);

public:
    /// Construct.
    explicit Hello3DUI();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Create and initialize a Scene.
    void InitScene();
    /// Create and initialize a Window control.
    void InitWindow();
    /// Create and add various common controls for demonstration purposes.
    void InitControls();
    /// Create a draggable fish button.
    void CreateDraggableFish();
    /// Handle drag begin for the fish button.
    void HandleDragBegin(StringHash eventType, VariantMap& eventData);
    /// Handle drag move for the fish button.
    void HandleDragMove(StringHash eventType, VariantMap& eventData);
    /// Handle drag end for the fish button.
    void HandleDragEnd(StringHash eventType, VariantMap& eventData);
    /// Handle any UI control being clicked.
    void HandleControlClicked(StringHash eventType, VariantMap& eventData);
    /// Handle close button pressed and released.
    void HandleClosePressed(StringHash eventType, VariantMap& eventData);
    /// Animate cube.
    void HandleUpdate(StringHash, VariantMap& eventData);
    /// Create 3D UI.
    void Init3DUI();

    /// The Scene.
    SharedPtr<Scene> scene_;
    /// The Window.
    SharedPtr<Window> window_;
    /// The UI's root UiElement.
    SharedPtr<UiElement> uiRoot_;
    /// Remembered drag begin position.
    IntVector2 dragBeginPosition_;
    /// Root UI element of texture.
    SharedPtr<UiElement> textureRoot_;
    /// UI element with instructions.
    SharedPtr<Text> instructions_;
    /// Enable or disable cube rotation.
    bool animateCube_;
    /// Enable or disable rendering to texture.
    bool renderOnCube_;
    /// Draw debug information of last clicked element.
    bool drawDebug_;
    /// Last clicked UI element.
    WeakPtr<UiElement> current_;
};


