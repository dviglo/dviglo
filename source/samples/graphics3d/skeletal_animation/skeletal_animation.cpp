// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/animated_model.h>
#include <dviglo/graphics/animation.h>
#include <dviglo/graphics/animation_state.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/debug_renderer.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/light.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/zone.h>
#include <dviglo/graphics/render_path.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/scene.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "mover.h"
#include "skeletal_animation.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(SkeletalAnimation)

SkeletalAnimation::SkeletalAnimation() :
    draw_debug_(false)
{
    // Register an object factory for our custom Mover component so that we can create them to scene nodes
    DV_CONTEXT.RegisterFactory<Mover>();
}

void SkeletalAnimation::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    create_scene();

    // Create the UI content
    create_instructions();

    // Setup the viewport for displaying the scene
    setup_viewport();

    // Hook up to the frame update and render post-update events
    subscribe_to_events();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_ABSOLUTE);
}

void SkeletalAnimation::create_scene()
{
    ResourceCache& cache = DV_RES_CACHE;

    // Включаем автоматическую перезагрузку изменённых ресурсов для тестирования FileWatcher.
    // Пользователь может редактировать шейдеры и сразу видеть результат
    cache.SetAutoReloadResources(true);

    scene_ = new Scene();

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Also create a DebugRenderer component so that we can draw debug geometry
    scene_->create_component<Octree>();
    scene_->create_component<DebugRenderer>();

    // Create scene node & StaticModel component for showing a static plane
    Node* planeNode = scene_->create_child("Plane");
    planeNode->SetScale(Vector3(50.0f, 1.0f, 50.0f));
    auto* planeObject = planeNode->create_component<StaticModel>();
    planeObject->SetModel(cache.GetResource<Model>("models/plane.mdl"));
    planeObject->SetMaterial(cache.GetResource<Material>("materials/stone_tiled.xml"));

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->create_child("Zone");
    auto* zone = zoneNode->create_component<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.5f, 0.5f, 0.5f));
    zone->SetFogColor(Color(0.4f, 0.5f, 0.8f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode = scene_->create_child("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    auto* light = lightNode->create_component<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetColor(Color(0.5f, 0.5f, 0.5f));
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    // Create animated models
    const unsigned NUM_MODELS = 30;
    const float MODEL_MOVE_SPEED = 2.0f;
    const float MODEL_ROTATE_SPEED = 100.0f;
    const BoundingBox bounds(Vector3(-20.0f, 0.0f, -20.0f), Vector3(20.0f, 0.0f, 20.0f));

    for (unsigned i = 0; i < NUM_MODELS; ++i)
    {
        Node* modelNode = scene_->create_child("Jill");
        modelNode->SetPosition(Vector3(Random(40.0f) - 20.0f, 0.0f, Random(40.0f) - 20.0f));
        modelNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));

        auto* modelObject = modelNode->create_component<AnimatedModel>();
        modelObject->SetModel(cache.GetResource<Model>("models/Kachujin/kachujin.mdl"));
        modelObject->SetMaterial(cache.GetResource<Material>("models/Kachujin/materials/kachujin.xml"));
        modelObject->SetCastShadows(true);

        // Create an AnimationState for a walk animation. Its time position will need to be manually updated to advance the
        // animation, The alternative would be to use an AnimationController component which updates the animation automatically,
        // but we need to update the model's position manually in any case
        auto* walkAnimation = cache.GetResource<Animation>("models/Kachujin/kachujin_walk.ani");

        AnimationState* state = modelObject->AddAnimationState(walkAnimation);
        // The state would fail to create (return null) if the animation was not found
        if (state)
        {
            // Enable full blending weight and looping
            state->SetWeight(1.0f);
            state->SetLooped(true);
            state->SetTime(Random(walkAnimation->GetLength()));
        }

        // Create our custom Mover component that will move & animate the model during each frame's update
        auto* mover = modelNode->create_component<Mover>();
        mover->SetParameters(MODEL_MOVE_SPEED, MODEL_ROTATE_SPEED, bounds);
#ifdef DV_GLES3
        Node* nLight = modelNode->create_child("Light", LOCAL);
        nLight->SetPosition(Vector3(1.0f, 2.0f, 1.0f));
        nLight->LookAt(Vector3::ZERO, Vector3::UP, TransformSpace::Parent);
        Light* light = nLight->create_component<Light>();
        light->SetLightType(LIGHT_SPOT);
        light->SetColor(Color(0.5f + Random(0.5f), 0.5f + Random(0.5f), 0.5f + Random(0.5f)));
#endif
    }

    // Create the camera. Limit far clip distance to match the fog
    cameraNode_ = scene_->create_child("Camera");
    auto* camera = cameraNode_->create_component<Camera>();
    camera->SetFarClip(300.0f);

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
#ifdef DV_GLES3
    CreateLights();
#endif
}

#ifdef DV_GLES3
void SkeletalAnimation::CreateLights() {
    for (unsigned i = 0; i < 40; i++) {
        Node* nLight = scene_->create_child("Light", LOCAL);
        Vector3 pos(Random(40.0f) - 20.0f, 1.0f + Random(1.0f), Random(40.0f) - 20.0f);
        nLight->SetPosition(pos);
        pos.y_ = 0;
        pos.x_ += Random(2.0f) - 1.0f;
        pos.z_ += Random(2.0f) - 1.0f;
        nLight->LookAt(pos);

        Light* light = nLight->create_component<Light>();
        light->SetLightType(LIGHT_SPOT);
        light->SetColor(Color(0.5f + Random(0.5f), 0.5f + Random(0.5f), 0.5f + Random(0.5f)));
    }
}
#endif

void SkeletalAnimation::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI.GetRoot()->create_child<Text>();
    instructionText->SetText(
        "Use WASD keys and mouse to move\n"
        "Space to toggle debug geometry\n"
        "\n"
        "Для ResourceCache включён FileWatcher:\n"
        "редактируйте шейдеры и сразу увидите результат"
    );
    instructionText->SetFont(DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf"), 15);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI.GetRoot()->GetHeight() / 4);
}

void SkeletalAnimation::setup_viewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
#ifdef DV_GLES3
        SharedPtr<RenderPath> rp(new RenderPath);
        rp->Load(DV_RES_CACHE.GetResource<XmlFile>("render_paths/deferred.xml"));
        viewport->SetRenderPath(rp);
#endif
    DV_RENDERER.SetViewport(0, viewport);
}

void SkeletalAnimation::subscribe_to_events()
{
    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(SkeletalAnimation, handle_update));

    // Subscribe handle_post_render_update() function for processing the post-render update event, sent after Renderer subsystem is
    // done with defining the draw calls for the viewports (but before actually executing them.) We will request debug geometry
    // rendering during that event
    subscribe_to_event(E_POSTRENDERUPDATE, DV_HANDLER(SkeletalAnimation, handle_post_render_update));
}

void SkeletalAnimation::move_camera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (DV_UI.GetFocusElement())
        return;

    Input& input = DV_INPUT;

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    IntVector2 mouseMove = input.GetMouseMove();
    yaw_ += MOUSE_SENSITIVITY * mouseMove.x;
    pitch_ += MOUSE_SENSITIVITY * mouseMove.y;
    pitch_ = Clamp(pitch_, -90.0f, 90.0f);

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input.GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    // Toggle debug geometry with space
    if (input.GetKeyPress(KEY_SPACE))
        draw_debug_ = !draw_debug_;
}

void SkeletalAnimation::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    move_camera(timeStep);
}

void SkeletalAnimation::handle_post_render_update(StringHash eventType, VariantMap& eventData)
{
    // If draw debug mode is enabled, draw viewport debug geometry, which will show eg. drawable bounding boxes and skeleton
    // bones. Note that debug geometry has to be separately requested each frame. Disable depth test so that we can see the
    // bones properly
    if (draw_debug_)
        DV_RENDERER.draw_debug_geometry(false);
}
