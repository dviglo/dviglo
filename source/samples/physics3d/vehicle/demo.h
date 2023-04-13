// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../../sample.h"

namespace dviglo
{

class Node;
class Scene;

}

class Vehicle;

/// Vehicle example.
/// This sample demonstrates:
///     - Creating a heightmap terrain with collision
///     - Constructing a physical vehicle with rigid bodies for the hull and the wheels, joined with constraints
///     - Defining attributes (including node and component references) of a custom component so that it can be saved and loaded
class VehicleDemo : public Sample
{
    DV_OBJECT(VehicleDemo);

public:
    /// Construct.
    explicit VehicleDemo();

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Create static scene content.
    void create_scene();
    /// Create the vehicle.
    void CreateVehicle();
    /// Construct an instruction text to the UI.
    void create_instructions();
    /// Subscribe to necessary events.
    void subscribe_to_events();
    /// Handle application update. Set controls to vehicle.
    void handle_update(StringHash eventType, VariantMap& eventData);
    /// Handle application post-update. Update camera position after vehicle has moved.
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

    /// The controllable vehicle component.
    WeakPtr<Vehicle> vehicle_;
};
