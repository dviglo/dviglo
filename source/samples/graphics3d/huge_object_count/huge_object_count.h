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

/// Huge object count example.
/// This sample demonstrates:
///     - Creating a scene with 250 x 250 simple objects
///     - Competing with http://yosoygames.com.ar/wp/2013/07/ogre-2-0-is-up-to-3x-faster/ :)
///     - Allowing examination of performance hotspots in the rendering code
///     - Using the profiler to measure the time taken to animate the scene
///     - Optionally speeding up rendering by grouping objects with the StaticModelGroup component
class HugeObjectCount : public Sample
{
    DV_OBJECT(HugeObjectCount, Sample);

public:
    /// Construct.
    explicit HugeObjectCount();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void create_scene();
    /// Construct an instruction text to the UI.
    void create_instructions();
    /// Set up a viewport for displaying the scene.
    void setup_viewport();
    /// Subscribe to application-wide logic update events.
    void subscribe_to_events();
    /// Read input and move the camera.
    void MoveCamera(float timeStep);
    /// Animate the scene.
    void AnimateObjects(float timeStep);
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);

    /// Box scene nodes.
    Vector<SharedPtr<Node>> boxNodes_;
    /// Animation flag.
    bool animate_;
    /// Group optimization flag.
    bool useGroups_;
};
