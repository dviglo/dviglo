// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
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
    auto* input = GetSubsystem<Input>();
    input->SetMouseVisible(true);

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Urho2DParticle::CreateScene()
{
    scene_ = new Scene();
    scene_->CreateComponent<Octree>();

    // Create camera node
    cameraNode_ = scene_->CreateChild("Camera");
    // Set camera's position
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));

    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetOrthographic(true);

    auto* graphics = GetSubsystem<Graphics>();
    camera->SetOrthoSize((float)graphics->GetHeight() * PIXEL_SIZE);
    camera->SetZoom(1.2f * Min((float)graphics->GetWidth() / 1280.0f, (float)graphics->GetHeight() / 800.0f)); // Set zoom according to user's resolution to ensure full visibility (initial zoom (1.2) is set for full visibility at 1280x800 resolution)

    auto* cache = GetSubsystem<ResourceCache>();
    auto* particleEffect = cache->GetResource<ParticleEffect2D>("Urho2D/sun.pex");
    if (!particleEffect)
        return;

    particleNode_ = scene_->CreateChild("ParticleEmitter2D");
    auto* particleEmitter = particleNode_->CreateComponent<ParticleEmitter2D>();
    particleEmitter->SetEffect(particleEffect);

    auto* greenSpiralEffect = cache->GetResource<ParticleEffect2D>("Urho2D/greenspiral.pex");
    if (!greenSpiralEffect)
        return;

    Node* greenSpiralNode = scene_->CreateChild("GreenSpiral");
    auto* greenSpiralEmitter = greenSpiralNode->CreateComponent<ParticleEmitter2D>();
    greenSpiralEmitter->SetEffect(greenSpiralEffect);
}

void Urho2DParticle::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText("Use mouse/touch to move the particle.");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void Urho2DParticle::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void Urho2DParticle::SubscribeToEvents()
{
    SubscribeToEvent(E_MOUSEMOVE, DV_HANDLER(Urho2DParticle, HandleMouseMove));
    if (touchEnabled_)
        SubscribeToEvent(E_TOUCHMOVE, DV_HANDLER(Urho2DParticle, HandleMouseMove));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void Urho2DParticle::HandleMouseMove(StringHash eventType, VariantMap& eventData)
{
    if (particleNode_)
    {
        using namespace MouseMove;
        auto x = (float)eventData[P_X].GetI32();
        auto y = (float)eventData[P_Y].GetI32();
        auto* graphics = GetSubsystem<Graphics>();
        auto* camera = cameraNode_->GetComponent<Camera>();
        particleNode_->SetPosition(camera->ScreenToWorldPoint(Vector3(x / graphics->GetWidth(), y / graphics->GetHeight(), 10.0f)));
    }
}
