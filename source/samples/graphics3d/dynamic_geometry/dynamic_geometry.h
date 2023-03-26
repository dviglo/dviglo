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

/// Dynamic geometry example.
/// This sample demonstrates:
///     - Cloning a Model resource
///     - Modifying the vertex buffer data of the cloned models at runtime to efficiently animate them
///     - Creating a Model resource and its buffer data from scratch
class DynamicGeometry : public Sample
{
    DV_OBJECT(DynamicGeometry, Sample);

public:
    /// Construct.
    explicit DynamicGeometry();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();
    /// Read input and move the camera.
    void MoveCamera(float timeStep);
    /// Animate the vertex data of the objects.
    void AnimateObjects(float timeStep);
    /// Handle the logic update event.
    void handle_update(StringHash eventType, VariantMap& eventData);

    /// Cloned models' vertex buffers that we will animate.
    Vector<SharedPtr<VertexBuffer>> animatingBuffers_;
    /// Original vertex positions for the sphere model.
    Vector<Vector3> originalVertices_;
    /// If the vertices are duplicates, indices to the original vertices (to allow seamless animation.)
    Vector<i32> vertexDuplicates_;
    /// Animation flag.
    bool animate_;
    /// Animation's elapsed time.
    float time_;
};
