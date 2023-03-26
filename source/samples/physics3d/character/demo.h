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

class Character;

/// Moving character example.
/// This sample demonstrates:
///     - Controlling a humanoid character through physics
///     - Driving animations using the AnimationController component
///     - Manual control of a bone scene node
///     - Implementing 1st and 3rd person cameras, using raycasts to avoid the 3rd person camera clipping into scenery
///     - Defining attributes of a custom component so that it can be saved and loaded
class CharacterDemo : public Sample
{
    DV_OBJECT(CharacterDemo, Sample);

public:
    /// Construct.
    explicit CharacterDemo();
    /// Destruct.
    ~CharacterDemo() override;

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Create static scene content.
    void CreateScene();
    /// Create controllable character.
    void CreateCharacter();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Subscribe to necessary events.
    void SubscribeToEvents();
    /// Handle application update. Set controls to character.
    void handle_update(StringHash eventType, VariantMap& eventData);
    /// Handle application post-update. Update camera position after character has moved.
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

    /// The controllable character component.
    WeakPtr<Character> character_;
    /// First person camera flag.
    bool firstPerson_;
};
