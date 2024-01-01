// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/debug_renderer.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/light.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/render_path.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "multiple_viewports.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(MultipleViewports)

MultipleViewports::MultipleViewports() :
    draw_debug_(false)
{
}

void MultipleViewports::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    create_scene();

    // Create the UI content
    create_instructions();

    // Setup the viewports for displaying the scene
    SetupViewports();

    // Hook up to the frame update and render post-update events
    subscribe_to_events();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_ABSOLUTE);
}

void MultipleViewports::create_scene()
{
    ResourceCache* cache = DV_RES_CACHE;

    scene_ = new Scene();

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Also create a DebugRenderer component so that we can draw debug geometry
    scene_->create_component<Octree>();
    scene_->create_component<DebugRenderer>();

    // Create scene node & StaticModel component for showing a static plane
    Node* planeNode = scene_->create_child("Plane");
    planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
    auto* planeObject = planeNode->create_component<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("models/plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("materials/stone_tiled.xml"));

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->create_child("Zone");
    auto* zone = zoneNode->create_component<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode = scene_->create_child("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    auto* light = lightNode->create_component<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    // Create some mushrooms
    const unsigned NUM_MUSHROOMS = 240;
    for (unsigned i = 0; i < NUM_MUSHROOMS; ++i)
    {
        Node* mushroomNode = scene_->create_child("Mushroom");
        mushroomNode->SetPosition(Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f));
        mushroomNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
        mushroomNode->SetScale(0.5f + Random(2.0f));
        auto* mushroomObject = mushroomNode->create_component<StaticModel>();
        mushroomObject->SetModel(cache->GetResource<Model>("models/mushroom.mdl"));
        mushroomObject->SetMaterial(cache->GetResource<Material>("materials/mushroom.xml"));
        mushroomObject->SetCastShadows(true);
    }

    // Create randomly sized boxes. If boxes are big enough, make them occluders
    const unsigned NUM_BOXES = 20;
    for (unsigned i = 0; i < NUM_BOXES; ++i)
    {
        Node* boxNode = scene_->create_child("Box");
        float size = 1.0f + Random(10.0f);
        boxNode->SetPosition(Vector3(Random(80.0f) - 40.0f, size * 0.5f, Random(80.0f) - 40.0f));
        boxNode->SetScale(size);
        auto* boxObject = boxNode->create_component<StaticModel>();
        boxObject->SetModel(cache->GetResource<Model>("models/box.mdl"));
        boxObject->SetMaterial(cache->GetResource<Material>("materials/stone.xml"));
        boxObject->SetCastShadows(true);
        if (size >= 3.0f)
            boxObject->SetOccluder(true);
    }

    // Create the cameras. Limit far clip distance to match the fog
    cameraNode_ = scene_->create_child("Camera");
    auto* camera = cameraNode_->create_component<Camera>();
    camera->SetFarClip(300.0f);

    // Parent the rear camera node to the front camera node and turn it 180 degrees to face backward
    // Here, we use the angle-axis constructor for Quaternion instead of the usual Euler angles
    rearCameraNode_ = cameraNode_->create_child("RearCamera");
    rearCameraNode_->Rotate(Quaternion(180.0f, Vector3::UP));
    auto* rearCamera = rearCameraNode_->create_component<Camera>();
    rearCamera->SetFarClip(300.0f);
    // Because the rear viewport is rather small, disable occlusion culling from it. Use the camera's
    // "view override flags" for this. We could also disable eg. shadows or force low material quality
    // if we wanted
    rearCamera->SetViewOverrideFlags(VO_DISABLE_OCCLUSION);

    // Set an initial position for the front camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
}

void MultipleViewports::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI->GetRoot()->create_child<Text>();
    instructionText->SetText(
        "Use WASD keys and mouse to move\n"
        "B to toggle bloom, F to toggle FXAA\n"
        "G для переключения grayscale\n"
        "Space to toggle debug geometry\n"
    );
    instructionText->SetFont(DV_RES_CACHE->GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI->GetRoot()->GetHeight() / 4);
}

void MultipleViewports::SetupViewports()
{
    DV_RENDERER->SetNumViewports(2);

    // Set up the front camera viewport
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER->SetViewport(0, viewport);

    // Clone the default render path so that we do not interfere with the other viewport, then add
    // bloom and FXAA postprocess effects to the front viewport. Render path commands can be tagged
    // for example with the effect name to allow easy toggling on and off. We start with the effects
    // disabled.
    SharedPtr<RenderPath> effectRenderPath = viewport->GetRenderPath()->Clone();
    effectRenderPath->Append(DV_RES_CACHE->GetResource<XmlFile>("postprocess/bloom.xml"));
    effectRenderPath->Append(DV_RES_CACHE->GetResource<XmlFile>("postprocess/fxaa2.xml"));
    effectRenderPath->Append(DV_RES_CACHE->GetResource<XmlFile>("postprocess/grayscale.xml"));
    // Make the bloom mixing parameter more pronounced
    effectRenderPath->SetShaderParameter("BloomMix", Vector2(0.9f, 0.6f));
    effectRenderPath->SetEnabled("bloom", false);
    effectRenderPath->SetEnabled("fxaa2", false);
    effectRenderPath->SetEnabled("grayscale", false);
    viewport->SetRenderPath(effectRenderPath);

    // Set up the rear camera viewport on top of the front view ("rear view mirror")
    // The viewport index must be greater in that case, otherwise the view would be left behind
    SharedPtr<Viewport> rearViewport(new Viewport(scene_, rearCameraNode_->GetComponent<Camera>(),
        IntRect(DV_GRAPHICS->GetWidth() * 2 / 3, 32, DV_GRAPHICS->GetWidth() - 32, DV_GRAPHICS->GetHeight() / 3)));
    DV_RENDERER->SetViewport(1, rearViewport);
}

void MultipleViewports::subscribe_to_events()
{
    // Subscribe handle_update() method for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(MultipleViewports, handle_update));

    // Subscribe handle_post_render_update() method for processing the post-render update event, during which we request
    // debug geometry
    subscribe_to_event(E_POSTRENDERUPDATE, DV_HANDLER(MultipleViewports, handle_post_render_update));
}

void MultipleViewports::move_camera(float timeStep)
{
     // Do not move if the UI has a focused element (the console)
    if (DV_UI->GetFocusElement())
        return;

    Input* input = DV_INPUT;

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    IntVector2 mouseMove = input->GetMouseMove();
    yaw_ += MOUSE_SENSITIVITY * mouseMove.x;
    pitch_ += MOUSE_SENSITIVITY * mouseMove.y;
    pitch_ = Clamp(pitch_, -90.0f, 90.0f);

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    // Toggle post processing effects on the front viewport. Note that the rear viewport is unaffected
    RenderPath* effectRenderPath = DV_RENDERER->GetViewport(0)->GetRenderPath();

    if (input->GetKeyPress(KEY_B))
        effectRenderPath->ToggleEnabled("bloom");

    if (input->GetKeyPress(KEY_F))
        effectRenderPath->ToggleEnabled("fxaa2");

    if (input->GetKeyPress(KEY_G))
        effectRenderPath->ToggleEnabled("grayscale");

    // Toggle debug geometry with space
    if (input->GetKeyPress(KEY_SPACE))
        draw_debug_ = !draw_debug_;
}

void MultipleViewports::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    move_camera(timeStep);
}

void MultipleViewports::handle_post_render_update(StringHash eventType, VariantMap& eventData)
{
    // If draw debug mode is enabled, draw viewport debug geometry, which will show eg. drawable bounding boxes and skeleton
    // bones. Disable depth test so that we can see the effect of occlusion
    if (draw_debug_)
        DV_RENDERER->draw_debug_geometry(false);
}
