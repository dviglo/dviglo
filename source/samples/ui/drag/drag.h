// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../Sample.h"

namespace dviglo
{
    class Node;
    class Scene;
}

/// GUI test example.
/// This sample demonstrates:
///     - Creating GUI elements from C++
///     - Loading GUI Style from xml
///     - Subscribing to GUI drag events and handling them
///     - Working with GUI elements with specific tags.
class UIDrag : public Sample
{
    DV_OBJECT(UIDrag, Sample);

public:
    /// Construct.
    explicit UIDrag();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    String GetScreenJoystickPatchString() const override { return
        "<patch>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Hat0']]\">"
        "        <attribute name=\"Is Visible\" value=\"false\" />"
        "    </add>"
        "</patch>";
    }

private:
    /// Construct the GUI.
    void CreateGUI();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();

    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    void HandleClick(StringHash eventType, VariantMap& eventData);
    void HandleDragBegin(StringHash eventType, VariantMap& eventData);
    void HandleDragMove(StringHash eventType, VariantMap& eventData);
    void HandleDragCancel(StringHash eventType, VariantMap& eventData);
};
