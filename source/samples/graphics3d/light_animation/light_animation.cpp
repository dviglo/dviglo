// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#include <dviglo/core/core_events.h>
#include <dviglo/engine/engine.h>
#include <dviglo/graphics/camera.h>
#include <dviglo/graphics/graphics.h>
#include <dviglo/graphics/material.h>
#include <dviglo/graphics/model.h>
#include <dviglo/graphics/octree.h>
#include <dviglo/graphics/renderer.h>
#include <dviglo/graphics/static_model.h>
#include <dviglo/input/input.h>
#include <dviglo/resource/resource_cache.h>
#include <dviglo/scene/object_animation.h>
#include <dviglo/scene/scene.h>
#include <dviglo/scene/value_animation.h>
#include <dviglo/ui/font.h>
#include <dviglo/ui/sprite.h>
#include <dviglo/ui/text.h>
#include <dviglo/ui/ui.h>

#include "light_animation.h"

#include <dviglo/common/debug_new.h>

DV_DEFINE_APPLICATION_MAIN(LightAnimation)

LightAnimation::LightAnimation()
{
}

void LightAnimation::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the UI content
    create_instructions();

    // Create the scene content
    create_scene();

    // Setup the viewport for displaying the scene
    setup_viewport();

    // Hook up to the frame update events
    subscribe_to_events();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void LightAnimation::create_scene()
{
    ResourceCache& cache = DV_RES_CACHE;

    scene_ = new Scene();

    // Create the Octree component to the scene. This is required before adding any drawable components, or else nothing will
    // show up. The default octree volume will be from (-1000, -1000, -1000) to (1000, 1000, 1000) in world coordinates; it
    // is also legal to place objects outside the volume but their visibility can then not be checked in a hierarchically
    // optimizing manner
    scene_->create_component<Octree>();

    // Create a child scene node (at world origin) and a StaticModel component into it. Set the StaticModel to show a simple
    // plane mesh with a "stone" material. Note that naming the scene nodes is optional. Scale the scene node larger
    // (100 x 100 world units)
    Node* planeNode = scene_->create_child("Plane");
    planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
    auto* planeObject = planeNode->create_component<StaticModel>();
    planeObject->SetModel(cache.GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache.GetResource<Material>("Materials/StoneTiled.xml"));

    // Create a point light to the world so that we can see something.
    Node* lightNode = scene_->create_child("PointLight");
    auto* light = lightNode->create_component<Light>();
    light->SetLightType(LIGHT_POINT);
    light->SetRange(10.0f);

    // Create light animation
    SharedPtr<ObjectAnimation> lightAnimation(new ObjectAnimation());

    // Create light position animation
    SharedPtr<ValueAnimation> positionAnimation(new ValueAnimation());
    // Use spline interpolation method
    positionAnimation->SetInterpolationMethod(IM_SPLINE);
    // Set spline tension
    positionAnimation->SetSplineTension(0.7f);
    positionAnimation->SetKeyFrame(0.0f, Vector3(-30.0f, 5.0f, -30.0f));
    positionAnimation->SetKeyFrame(1.0f, Vector3( 30.0f, 5.0f, -30.0f));
    positionAnimation->SetKeyFrame(2.0f, Vector3( 30.0f, 5.0f,  30.0f));
    positionAnimation->SetKeyFrame(3.0f, Vector3(-30.0f, 5.0f,  30.0f));
    positionAnimation->SetKeyFrame(4.0f, Vector3(-30.0f, 5.0f, -30.0f));
    // Set position animation
    lightAnimation->AddAttributeAnimation("Position", positionAnimation);

    // Create text animation
    SharedPtr<ValueAnimation> textAnimation(new ValueAnimation());
    textAnimation->SetKeyFrame(0.0f, "WHITE");
    textAnimation->SetKeyFrame(1.0f, "RED");
    textAnimation->SetKeyFrame(2.0f, "YELLOW");
    textAnimation->SetKeyFrame(3.0f, "GREEN");
    textAnimation->SetKeyFrame(4.0f, "WHITE");
    DV_UI.GetRoot()->GetChild(String("animatingText"))->SetAttributeAnimation("Text", textAnimation);

    // Create UI element animation
    // (note: a spritesheet and "Image Rect" attribute should be used in real use cases for better performance)
    SharedPtr<ValueAnimation> spriteAnimation(new ValueAnimation());
    spriteAnimation->SetKeyFrame(0.0f, ResourceRef("Texture2D", "Urho2D/GoldIcon/1.png"));
    spriteAnimation->SetKeyFrame(0.1f, ResourceRef("Texture2D", "Urho2D/GoldIcon/2.png"));
    spriteAnimation->SetKeyFrame(0.2f, ResourceRef("Texture2D", "Urho2D/GoldIcon/3.png"));
    spriteAnimation->SetKeyFrame(0.3f, ResourceRef("Texture2D", "Urho2D/GoldIcon/4.png"));
    spriteAnimation->SetKeyFrame(0.4f, ResourceRef("Texture2D", "Urho2D/GoldIcon/5.png"));
    spriteAnimation->SetKeyFrame(0.5f, ResourceRef("Texture2D", "Urho2D/GoldIcon/1.png"));
    DV_UI.GetRoot()->GetChild(String("animatingSprite"))->SetAttributeAnimation("Texture", spriteAnimation);

    // Create light color animation
    SharedPtr<ValueAnimation> colorAnimation(new ValueAnimation());
    colorAnimation->SetKeyFrame(0.0f, Color::WHITE);
    colorAnimation->SetKeyFrame(1.0f, Color::RED);
    colorAnimation->SetKeyFrame(2.0f, Color::YELLOW);
    colorAnimation->SetKeyFrame(3.0f, Color::GREEN);
    colorAnimation->SetKeyFrame(4.0f, Color::WHITE);
    // Set Light component's color animation
    lightAnimation->AddAttributeAnimation("@Light/Color", colorAnimation);

    // Apply light animation to light node
    lightNode->SetObjectAnimation(lightAnimation);

    // Create more StaticModel objects to the scene, randomly positioned, rotated and scaled. For rotation, we construct a
    // quaternion from Euler angles where the Y angle (rotation about the Y axis) is randomized. The mushroom model contains
    // LOD levels, so the StaticModel component will automatically select the LOD level according to the view distance (you'll
    // see the model get simpler as it moves further away). Finally, rendering a large number of the same object with the
    // same material allows instancing to be used, if the GPU supports it. This reduces the amount of CPU work in rendering the
    // scene.
    const unsigned NUM_OBJECTS = 200;
    for (unsigned i = 0; i < NUM_OBJECTS; ++i)
    {
        Node* mushroomNode = scene_->create_child("Mushroom");
        mushroomNode->SetPosition(Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f));
        mushroomNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
        mushroomNode->SetScale(0.5f + Random(2.0f));
        auto* mushroomObject = mushroomNode->create_component<StaticModel>();
        mushroomObject->SetModel(cache.GetResource<Model>("Models/Mushroom.mdl"));
        mushroomObject->SetMaterial(cache.GetResource<Material>("Materials/Mushroom.xml"));
    }

    // Create a scene node for the camera, which we will move around
    // The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
    cameraNode_ = scene_->create_child("Camera");
    cameraNode_->create_component<Camera>();

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
}

void LightAnimation::create_instructions()
{
    // Construct new Text object, set string to display and font to use
    auto* instructionText = DV_UI.GetRoot()->create_child<Text>();
    instructionText->SetText("Use WASD keys and mouse to move");
    auto* font = DV_RES_CACHE.GetResource<Font>("fonts/anonymous pro.ttf");
    instructionText->SetFont(font, 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, DV_UI.GetRoot()->GetHeight() / 4);

    // Animating text
    auto* text = DV_UI.GetRoot()->create_child<Text>("animatingText");
    text->SetFont(font, 15);
    text->SetHorizontalAlignment(HA_CENTER);
    text->SetVerticalAlignment(VA_CENTER);
    text->SetPosition(0, DV_UI.GetRoot()->GetHeight() / 4 + 20);

    // Animating sprite in the top left corner
    auto* sprite = DV_UI.GetRoot()->create_child<Sprite>("animatingSprite");
    sprite->SetPosition(8, 8);
    sprite->SetSize(64, 64);
}

void LightAnimation::setup_viewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    SharedPtr<Viewport> viewport(new Viewport(scene_, cameraNode_->GetComponent<Camera>()));
    DV_RENDERER.SetViewport(0, viewport);
}

void LightAnimation::move_camera(float timeStep)
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
    // Use the Translate() function (default local space) to move relative to the node's orientation.
    if (input.GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input.GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
}

void LightAnimation::subscribe_to_events()
{
    // Subscribe handle_update() function for processing update events
    subscribe_to_event(E_UPDATE, DV_HANDLER(LightAnimation, handle_update));
}

void LightAnimation::handle_update(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    move_camera(timeStep);
}
