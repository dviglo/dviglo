// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../Sample.h"

namespace dviglo
{

class Node;
class Scene;
class RibbonTrail;

}

/// Ribbon trail demo.
/// This sample demonstrates how to use both trail types of RibbonTrail component.
class RibbonTrailDemo : public Sample
{
    DV_OBJECT(RibbonTrailDemo, Sample);

public:
    /// Construct.
    explicit RibbonTrailDemo();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

protected:
    /// Trail that emitted from sword.
    SharedPtr<RibbonTrail> swordTrail_;
    /// Animation controller of the ninja.
    SharedPtr<AnimationController> ninjaAnimCtrl_;
    /// The time sword start emitting trail.
    float swordTrailStartTime_;
    /// The time sword stop emitting trail.
    float swordTrailEndTime_;
    /// Box node 1.
    SharedPtr<Node> boxNode1_;
    /// Box node 2.
    SharedPtr<Node> boxNode2_;
    /// Sum of timestep.
    float timeStepSum_;

private:
    /// Construct the scene content.
    void create_scene();
    /// Construct an instruction text to the UI.
    void create_instructions();
    /// Set up a viewport for displaying the scene.
    void setup_viewport();
    /// Read input and moves the camera.
    void MoveCamera(float timeStep);
    /// Subscribe to application-wide logic update events.
    void subscribe_to_events();
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);

};
