// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/urho_2d/particle_effect_2d.h>
#include <dviglo/urho_2d/particle_emitter_2d.h>

#include "particle.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(Urho2DParticle)

Urho2DParticle::Urho2DParticle()
{
}

void Urho2DParticle::Start()
{
    // Execute base class startup
    Sample::Start();

    // Set mouse visible
    DV_INPUT->SetMouseVisible(true);

    // Create the scene content
    create_scene();

    // Create the UI content
    create_instructions();

    // Setup the viewport for displaying the scene
    setup_viewport();

    // Hook up to the frame update events
    subscribe_to_events();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Urho2DParticle::create_scene()
{
    scene_ = new Scene();
    scene_->create_component<Octree>();

    // Create camera node
    cameraNode_ = scene_->create_child("Camera");
    // Set camera's position
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

    auto* camera = cameraNode_->create_component<Camera>();
    camera->SetOrthographic(true);

    camera->SetOrthoSize((float)DV_GRAPHICS->GetHeight() * PIXEL_SIZE);
    camera->SetZoom(1.2f * Min((float)DV_GRAPHICS->GetWidth() / 1280.0f, (float)DV_GRAPHICS->GetHeight() / 800.0f)); // Set zoom according to user's resolution to ensure full visibility (initial zoom (1.2) is set for full visibility at 1280x800 resolution)

    auto* particleEffect = DV_RES_CACHE->GetResource<ParticleEffect2D>("sprites/sun.pex");
    if (!particleEffect)
        return;

    particleNode_ = scene_->create_child("ParticleEmitter2D");
    auto* particleEmitter = particleNode_->create_component<ParticleEmitter2D>();
    particleEmitter->SetEffect(particleEffect);

    auto* greenSpiralEffect = DV_RES_CACHE->GetResource<ParticleEffect2D>("sprites/greenspiral.pex");
    if (!greenSpiralEffect)
        return;

    Node* greenSpiralNode = scene_->create_child("GreenSpiral");
    auto* greenSpiralEmitter = greenSpiralNode->create_component<ParticleEmitter2D>();
    greenSpiralEmitter->SetEffect(greenSpiralEffect);
}

void Urho2DParticle::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI->GetRoot()->create_child<Text>();
    instructionText->SetText("Use mouse to move the particle.");
    instructionText->SetFont(DV_RES_CACHE->GetResource<Font>("fonts/anonymous pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI->GetRoot()->GetHeight() / 4);
}

void Urho2DParticle::setup_viewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER->SetViewport(0, viewport);
}

void Urho2DParticle::subscribe_to_events()
{
    subscribe_to_event(E_MOUSEMOVE, DV_HANDLER(Urho2DParticle, HandleMouseMove));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    unsubscribe_from_event(E_SCENEUPDATE);
}

void Urho2DParticle::HandleMouseMove(StringHash eventType, VariantMap& eventData)
{
    if (particleNode_)
    {
        using namespace MouseMove;
        auto x = (float)eventData[P_X].GetI32();
        auto y = (float)eventData[P_Y].GetI32();
        auto* camera = cameraNode_->GetComponent<Camera>();
        particleNode_->SetPosition(camera->screen_to_world_point(Vector3(x / DV_GRAPHICS->GetWidth(), y / DV_GRAPHICS->GetHeight(), 10.0f)));
    }
}
